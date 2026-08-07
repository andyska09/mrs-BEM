#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "mybem/cma.h"
#include "mybem/csv.h"
#include "mybem/drone.h"
#include "mybem/model.h"
#include "mybem/yaml.h"

using namespace mybem;
using Eigen::Array3d;
using Eigen::VectorXd;

namespace
{

    /* Merged flight CSV, FLU. Column order per NeuroBEM/README.md. */
    constexpr size_t kMergedCols = 29;
    constexpr size_t kAngAcc = 1;
    constexpr size_t kAngVel = 4;
    constexpr size_t kAcc = 11;
    constexpr size_t kVel = 14;
    constexpr size_t kMot = 20;
    constexpr size_t kDMot = 24;

    constexpr double kSigmaStop = 1e-4;
    constexpr double kPenalty = 1e3;

    [[noreturn]] void fail(const std::string &msg)
    {
        printf("tune: %s\n", msg.c_str());
        exit(1);
    }

    int threadId()
    {
#ifdef _OPENMP
        return omp_get_thread_num();
#else
        return 0;
#endif
    }

    struct Sample
    {
        State s;
        Eigen::Vector3d fmeas, tmeas; // FRD, like the model output
    };

    std::vector<Sample> load(const std::string &path, const Drone &drone)
    {
        CsvReader in(path);
        if (in.cols() < kMergedCols)
            fail(path + ": expected at least " + std::to_string(kMergedCols) +
                 " columns, found " + std::to_string(in.cols()));

        auto vec3 = [](const double *d, size_t i)
        {
            return Eigen::Vector3d{d[i], d[i + 1], d[i + 2]};
        };

        std::vector<Sample> out(in.rows());
        for (size_t i = 0; i < in.rows(); ++i)
        {
            const double *d = in.row(i);
            Sample &s = out[i];
            s.s.vel = flu2frd(vec3(d, kVel));
            s.s.omega = flu2frd(vec3(d, kAngVel));
            s.s.mot.assign(d + kMot, d + kMot + 4);
            s.s.dmot.assign(d + kDMot, d + kDMot + 4);
            const Wrench m =
                drone.measured(vec3(d, kAcc), vec3(d, kAngAcc), vec3(d, kAngVel));
            s.fmeas = flu2frd(m.force);
            s.tmeas = flu2frd(m.torque);
        }
        return out;
    }

#ifdef _OPENMP
#pragma omp declare reduction(+ : Array3d : omp_out += omp_in) \
    initializer(omp_priv = Array3d::Zero())
#endif

    /* Per-axis force/torque MSE over `data` at `p`, parallelized across samples
     * with one Model per thread. */
    void mse(std::vector<Model> &fleet, const Params &p,
             const std::vector<Sample> &data, Array3d &fmse, Array3d &tmse)
    {
        Array3d fse = Array3d::Zero(), tse = Array3d::Zero();
#ifdef _OPENMP
#pragma omp parallel reduction(+ : fse, tse)
#endif
        {
            Model &model = fleet[threadId()];
            model.set(p);
#ifdef _OPENMP
#pragma omp for nowait
#endif
            for (size_t i = 0; i < data.size(); ++i)
            {
                const Wrench w = model.evaluate(data[i].s);
                fse += (w.force - data[i].fmeas).array().square();
                tse += (w.torque - data[i].tmeas).array().square();
            }
        }
        fmse = fse / data.size();
        tmse = tse / data.size();
    }

    enum class Obj
    {
        Force,
        Torque,
        Both
    };

    const char *objName(Obj o)
    {
        return o == Obj::Force    ? "force"
               : o == Obj::Torque ? "torque"
                                  : "both";
    }

    double loss(std::vector<Model> &fleet, const std::vector<Sample> &data,
                const Params &p, double sf, double st, Obj obj)
    {
        Array3d fm, tm;
        mse(fleet, p, data, fm, tm);
        double l = 0;
        if (obj != Obj::Torque)
            l += fm.sum() / (sf * sf);
        if (obj != Obj::Force)
            l += tm.sum() / (st * st);
        return l;
    }

    /* Table-II row: {Fxy, Fz, F, Mxy, Mz, M}. */
    std::array<double, 6> rmse(const Array3d &fm, const Array3d &tm)
    {
        return {std::sqrt((fm[0] + fm[1]) / 2), std::sqrt(fm[2]),
                std::sqrt(fm.sum() / 3), std::sqrt((tm[0] + tm[1]) / 2),
                std::sqrt(tm[2]), std::sqrt(tm.sum() / 3)};
    }

    void printRmse(const std::array<double, 6> &m)
    {
        printf("           xy       z        total\n");
        printf("force  %8.4f %8.4f %8.4f  [N]\n", m[0], m[1], m[2]);
        printf("torque %8.5f %8.5f %8.5f  [Nm]\n", m[3], m[4], m[5]);
    }

    /* Maps CMA-ES's unbounded x-space onto the free tunables, centred on the
     * loaded model's current values so a tune can be resumed from its own output.
     * x=0 is that value, |x|=1 is half the parameter's range away. */
    class SearchSpace
    {
    public:
        SearchSpace(const std::vector<TunableParam> &free, const Params &start)
            : free_(free),
              centre_(free.size()),
              scale_(free.size()),
              xlo_(free.size()),
              xhi_(free.size())
        {
            for (size_t i = 0; i < free_.size(); ++i)
            {
                const TunableParam &t = free_[i];
                centre_[i] = start.at(t.key);
                if (centre_[i] < t.lo || centre_[i] > t.hi)
                    fail(t.key + " = " + fmt(centre_[i]) + " is outside its bounds [" +
                         fmt(t.lo) + ", " + fmt(t.hi) + "]");
                scale_[i] = (t.hi - t.lo) / 2;
                xlo_[i] = (t.lo - centre_[i]) / scale_[i];
                xhi_[i] = (t.hi - centre_[i]) / scale_[i];
            }
        }

        VectorXd clamp(const VectorXd &x) const
        {
            return x.cwiseMax(xlo_).cwiseMin(xhi_);
        }
        double penalty(const VectorXd &x) const { return (x - clamp(x)).squaredNorm(); }
        VectorXd params(const VectorXd &x) const
        {
            return centre_ + x.cwiseProduct(scale_);
        }
        Params toConfig(const VectorXd &x) const
        {
            Params c;
            const VectorXd p = params(x);
            for (size_t i = 0; i < free_.size(); ++i)
                c[free_[i].key] = p[i];
            return c;
        }

    private:
        std::vector<TunableParam> free_;
        VectorXd centre_, scale_, xlo_, xhi_;
    };

    double objective(std::vector<Model> &fleet, const std::vector<Sample> &data,
                     const SearchSpace &space, const VectorXd &x, double sf,
                     double st, Obj obj)
    {
        const double value =
            loss(fleet, data, space.toConfig(space.clamp(x)), sf, st, obj) +
            kPenalty * space.penalty(x);
        return std::isfinite(value) ? value : 1e12;
    }

    std::vector<int> rankByFitness(const std::vector<double> &fitness)
    {
        std::vector<int> order(fitness.size());
        for (size_t i = 0; i < order.size(); ++i)
            order[i] = (int)i;
        std::sort(order.begin(), order.end(),
                  [&](int a, int b)
                  { return fitness[a] < fitness[b]; });
        return order;
    }

    struct Args
    {
        std::string model, data, drone, out;
        std::vector<std::string> free;
        Obj obj = Obj::Force;
        int gens = 100;
        unsigned seed = 0;
        int threads = 0;
        bool list = false;
    };

    std::vector<std::string> split(const std::string &s, char sep)
    {
        std::vector<std::string> out;
        size_t a = 0;
        while (a <= s.size())
        {
            const size_t b = s.find(sep, a);
            const std::string tok = s.substr(a, b == std::string::npos ? b : b - a);
            if (!tok.empty())
                out.push_back(tok);
            if (b == std::string::npos)
                break;
            a = b + 1;
        }
        return out;
    }

    bool parseArgs(int argc, char **argv, Args &a)
    {
        if (argc < 3)
            return false;
        a.model = argv[1];
        a.data = argv[2];
        for (int i = 3; i < argc; ++i)
        {
            const std::string arg = argv[i];
            const bool has_value = i + 1 < argc;
            if (arg == "--list")
            {
                a.list = true;
            }
            else if (arg == "--drone" && has_value)
            {
                a.drone = argv[++i];
            }
            else if (arg == "--out" && has_value)
            {
                a.out = argv[++i];
            }
            else if (arg == "--free" && has_value)
            {
                a.free = split(argv[++i], ',');
            }
            else if (arg == "--gens" && has_value)
            {
                a.gens = std::atoi(argv[++i]);
            }
            else if (arg == "--seed" && has_value)
            {
                a.seed = (unsigned)std::atoi(argv[++i]);
            }
            else if (arg == "--threads" && has_value)
            {
                a.threads = std::atoi(argv[++i]);
            }
            else if (arg == "--loss" && has_value)
            {
                const std::string m = argv[++i];
                if (m == "force")
                    a.obj = Obj::Force;
                else if (m == "torque")
                    a.obj = Obj::Torque;
                else if (m == "both")
                    a.obj = Obj::Both;
                else
                    return false;
            }
            else
            {
                return false;
            }
        }
        return true;
    }

    /* `--free` takes tunable names. A bare name is accepted when exactly one
     * component offers it, so `lift_coefficient` works but an ambiguous name has
     * to be written `bem.lift_coefficient`. */
    std::vector<TunableParam> resolveFree(const std::vector<TunableParam> &all,
                                          const std::vector<std::string> &names)
    {
        if (names.size() == 1 && names[0] == "all")
            return all;

        std::vector<TunableParam> out;
        for (const std::string &name : names)
        {
            std::vector<const TunableParam *> hits;
            for (const TunableParam &t : all)
            {
                const size_t dot = t.key.rfind('.');
                if (t.key == name || t.key.substr(dot + 1) == name)
                    hits.push_back(&t);
            }
            if (hits.empty())
                fail("no tunable named '" + name + "' (try --list)");
            if (hits.size() > 1)
            {
                std::string cands;
                for (const TunableParam *t : hits)
                    cands += " " + t->key;
                fail("'" + name + "' is ambiguous:" + cands);
            }
            out.push_back(*hits[0]);
        }
        return out;
    }

    std::string baseName(const std::string &path)
    {
        const size_t end = path.find_last_not_of('/');
        if (end == std::string::npos)
            return path;
        const size_t slash = path.find_last_of('/', end);
        return path.substr(slash == std::string::npos ? 0 : slash + 1,
                           end - (slash == std::string::npos ? -1 : slash));
    }

    void writeMetrics(const std::string &path, const std::array<double, 6> &base,
                      const std::array<double, 6> &best)
    {
        std::ofstream os(path);
        os.precision(9);
        os << "config,Fxy,Fz,F,Mxy,Mz,M\n";
        const std::array<double, 6> *rows[] = {&base, &best};
        const char *labels[] = {"baseline", "best"};
        for (int r = 0; r < 2; ++r)
        {
            os << labels[r];
            for (double v : *rows[r])
                os << ',' << v;
            os << '\n';
        }
    }

    void writeRunRecord(const std::string &path, const Args &a, const Drone &drone,
                        const std::vector<TunableParam> &free, int threads,
                        double base_loss, double best_loss)
    {
        YamlWriter w(path);
        w.scalar("kind", "tune");
        w.scalar("model", a.model);
        w.scalar("data", a.data);
        w.scalar("drone", drone.name.empty() ? a.drone : drone.name);
        w.scalar("loss", objName(a.obj));
        w.scalar("generations", std::to_string(a.gens));
        w.scalar("seed", std::to_string(a.seed));
        w.scalar("threads", std::to_string(threads));
        std::string names = "[";
        for (size_t i = 0; i < free.size(); ++i)
            names += (i ? ", " : "") + free[i].key;
        w.scalar("free", names + "]");
        w.scalar("objective_baseline", fmt(base_loss));
        w.scalar("objective_best", fmt(best_loss));
    }

} // namespace

int main(int argc, char **argv)
{
    Args args;
    if (!parseArgs(argc, argv, args))
    {
        printf(
            "usage: mybem-tune MODEL.yaml DATA.csv --drone DRONE.yaml [options]\n"
            "  --list             print the model's tunables and exit\n"
            "  --free a,b,c|all   parameters to optimize (default: report only)\n"
            "  --loss force|torque|both     (default: force)\n"
            "  --gens N           generations (default: 100)\n"
            "  --seed N           CMA-ES seed (default: 0)\n"
            "  --threads N        OpenMP threads (default: all cores)\n"
            "  --out DIR          write model.yaml, convergence.csv, metrics.csv,\n"
            "                     tune.yaml\n");
        return 1;
    }

    if (args.list)
    {
        for (const TunableParam &t : Model::load(args.model).tunables())
            printf("%-40s %12.6g  [%g, %g]\n", t.key.c_str(), t.def, t.lo, t.hi);
        return 0;
    }
    if (args.drone.empty())
        fail("--drone is required");
    if (!args.free.empty() && args.out.empty())
        fail("--free needs --out");

    const Drone drone = Drone::load(args.drone);
    const std::vector<Sample> data = load(args.data, drone);

    /* Fitness is parallelized across samples, so each thread needs its own
     * Model. omp_get_num_procs respects the cpuset and ignores a PBS-injected
     * OMP_NUM_THREADS=1. */
    int threads = 1;
#ifdef _OPENMP
    omp_set_num_threads(args.threads > 0 ? args.threads : omp_get_num_procs());
    omp_set_max_active_levels(1);
#pragma omp parallel
#pragma omp single
    threads = omp_get_num_threads();
#endif
    std::vector<Model> fleet;
    for (int i = 0; i < threads; ++i)
        fleet.push_back(Model::load(args.model));

    const Params start = fleet[0].values();
    printf("%zu rows | %d thread%s | %zu components\n", data.size(), threads,
           threads == 1 ? "" : "s", fleet[0].size());

    Array3d bfm, btm;
    mse(fleet, start, data, bfm, btm);
    const double sf = std::sqrt(bfm.sum()), st = std::sqrt(btm.sum());
    const std::array<double, 6> base_metrics = rmse(bfm, btm);
    printf("--- baseline (%s) ---\n", args.model.c_str());
    printRmse(base_metrics);

    if (args.free.empty())
        return 0;

    const std::vector<TunableParam> free =
        resolveFree(fleet[0].tunables(), args.free);
    SearchSpace space(free, start);
    Cma optimizer((int)free.size(), args.seed);

    std::filesystem::create_directories(args.out);
    std::ofstream log(args.out + "/convergence.csv");
    log << "gen,loss";
    for (const TunableParam &t : free)
        log << ',' << t.key;
    log << '\n';

    // Each term is normalized by its own baseline MSE, so the start is exactly 1
    // per active term.
    const double base_loss =
        (args.obj != Obj::Torque) + (args.obj != Obj::Force);
    printf("--- CMA-ES: %zu free, loss=%s, %d gens, seed %u ---\n", free.size(),
           objName(args.obj), args.gens, args.seed);
    for (const TunableParam &t : free)
        printf("  %s\n", t.key.c_str());

    VectorXd best_x = optimizer.mean();
    double best_loss = base_loss;

    for (int gen = 0; gen < args.gens; ++gen)
    {
        const std::vector<Offspring> population = optimizer.sample();

        std::vector<double> fitness(population.size());
        for (size_t k = 0; k < population.size(); ++k)
            fitness[k] = objective(fleet, data, space, population[k].point, sf, st,
                                   args.obj);

        const std::vector<int> ranked = rankByFitness(fitness);
        optimizer.update(population, ranked, gen);

        if (fitness[ranked[0]] < best_loss)
        {
            best_loss = fitness[ranked[0]];
            best_x = space.clamp(population[ranked[0]].point);
        }

        log << gen << ',' << best_loss;
        const VectorXd p = space.params(best_x);
        for (Eigen::Index i = 0; i < p.size(); ++i)
            log << ',' << p[i];
        log << '\n';
        log.flush();

        if (gen % 10 == 0)
            printf("gen %3d  loss %.5f  sigma %.4f\n", gen, best_loss,
                   optimizer.sigma());
        if (optimizer.sigma() < kSigmaStop)
            break;
    }

    const Params best = space.toConfig(best_x);
    Array3d fm, tm;
    mse(fleet, best, data, fm, tm);
    const std::array<double, 6> best_metrics = rmse(fm, tm);
    printf("--- best (%.5f -> %.5f) ---\n", base_loss, best_loss);
    printRmse(best_metrics);

    fleet[0].set(best);
    fleet[0].setName(baseName(args.out));
    fleet[0].save(args.out + "/model.yaml");
    writeMetrics(args.out + "/metrics.csv", base_metrics, best_metrics);
    writeRunRecord(args.out + "/tune.yaml", args, drone, free, threads, base_loss,
                   best_loss);
    printf("written: %s\n", args.out.c_str());
    return 0;
}
