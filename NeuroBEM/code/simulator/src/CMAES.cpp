// CMA-ES BEM parameter fit on normalized force (+ optionally torque) MSE.
//   ./cmaes data.csv                 report at compile-time defaults
//   ./cmaes data.csv config.yaml     report at given config
//   ./cmaes data.csv --cma N [--joint]   fit the first N registry params,
//                                        writes best.yaml + convergence.csv
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include <eigen3/Eigen/Dense>

#include "config.h"
#include "params.h"
#include "quadcopter.h"

using Eigen::Array3d;
using Eigen::MatrixXd;
using Eigen::Vector3d;
using Eigen::VectorXd;

static constexpr double MASS = 0.772;
static const Vector3d INERTIA{0.00254, 0.00214, 0.00436};
static constexpr int MAXGEN = 300;

struct ParamSpec
{
    const char *key;
    const char *section;
    double def, lo, hi; // defaults must match params.h; pitch/twist in degrees
};

// ordered by optimization priority: --cma N frees the first N entries.
// `section` groups the key in the emitted YAML (single source of truth).
static const std::vector<ParamSpec> REGISTRY = {
    {"lift_coefficient", "bem", 15.24214, 0.5, 40},
    {"drag_coefficient", "bem", 13.54894, 0.5, 40},
    {"hinge_spring_constant", "bem", 5.89, 0.5, 30},
    {"lift_offset", "bem", 0, -0.2, 0.3},
    {"hforce_scale", "bem", 1, 0.1, 6},
    {"thrust_scale", "quad", 1, 0.5, 2},
    {"pitch", "bem", 21.77, 3, 40},
    {"twist", "bem", -11, -34, 6},
    {"chord_inner", "bem", 1.7e-2, 0.005, 0.04},
    {"chord_outer", "bem", 0.7e-2, 0.002, 0.03},
    {"horizontal_drag_coefficient", "body_drag", 1, 0.1, 5},
    {"vertical_drag_coefficient", "body_drag", 1, 0.1, 5},
    {"radius", "bem", 0.064770, 0.04, 0.09},
    {"dx", "quad", 0.078, 0.04, 0.15},
    {"dy", "quad", 0.1, 0.05, 0.2},
    {"dz", "quad", 0.027, -0.05, 0.1},
    {"frontarea_x", "body_drag", 0.06 * 0.09, 0, 0.03},
    {"frontarea_y", "body_drag", 0.1 * 0.09, 0, 0.03},
    {"frontarea_z", "body_drag", 0.1 * 0.06, 0, 0.03},
    // load-only tail: keep N below these so they stay at default
    {"num_blades", "bem", 3, 2, 4},
    {"air_density", "bem", 1.204, 1.0, 1.4},
};

// YAML section emission order
static const std::vector<const char *> SECTION_ORDER = {"bem", "quad",
                                                        "body_drag"};

static std::map<std::string, double> defaults()
{
    std::map<std::string, double> m;
    for (const ParamSpec &p : REGISTRY)
        m[p.key] = p.def;
    return m;
}

static void writeYaml(std::ostream &os, const std::map<std::string, double> &c)
{
    os << std::setprecision(9);
    for (const char *section : SECTION_ORDER)
    {
        os << section << ":\n";
        for (const ParamSpec &p : REGISTRY)
            if (std::string(p.section) == section)
                os << "  " << p.key << ": " << c.at(p.key) << '\n';
    }
}

struct Sample
{
    Vector3d ang_acc, ang_vel, acc, vel, pos, fmeas, tmeas;
    Eigen::Quaterniond quat;
    std::vector<double> omega;
};

namespace col
{
    constexpr int ANG_ACC = 1, ANG_VEL = 4, QUAT_XYZ = 7, QUAT_W = 10, ACC = 11,
                  VEL = 14, POS = 17, OMEGA = 20, MIN = 28;
}

static bool parseRow(const std::string &line, Sample &s)
{
    if (line.empty() || (!std::isdigit(line[0]) && line[0] != '-'))
        return false;
    std::stringstream ss(line);
    std::vector<double> v;
    std::string cell;
    while (std::getline(ss, cell, ','))
        v.push_back(std::stod(cell));
    if ((int)v.size() < col::MIN)
        return false;

    auto vec3 = [&](int i)
    { return Vector3d(v[i], v[i + 1], v[i + 2]); };
    s.ang_acc = vec3(col::ANG_ACC);
    s.ang_vel = vec3(col::ANG_VEL);
    s.quat = Eigen::Quaterniond(v[col::QUAT_W], v[col::QUAT_XYZ],
                                v[col::QUAT_XYZ + 1], v[col::QUAT_XYZ + 2]);
    s.acc = vec3(col::ACC);
    s.vel = vec3(col::VEL);
    s.pos = vec3(col::POS);
    s.omega = {v[col::OMEGA], v[col::OMEGA + 1], v[col::OMEGA + 2],
               v[col::OMEGA + 3]};
    s.fmeas = MASS * s.acc;
    s.tmeas = INERTIA.cwiseProduct(s.ang_acc) +
              s.ang_vel.cross(INERTIA.cwiseProduct(s.ang_vel));
    return true;
}

static std::vector<Sample> load(const std::string &path)
{
    std::ifstream file(path);
    if (!file)
    {
        fprintf(stderr, "cannot open %s\n", path.c_str());
        exit(1);
    }
    std::vector<Sample> out;
    std::string line;
    Sample s;
    while (std::getline(file, line))
        if (parseRow(line, s))
            out.push_back(s);
    return out;
}

static void mse(Quadcopter &quad, const std::vector<Sample> &data,
                Array3d &fmse, Array3d &tmse)
{
    Array3d fse = Array3d::Zero(), tse = Array3d::Zero();
    for (const Sample &s : data)
    {
        quad.setAcceleration(flu2frd(s.acc));
        quad.setVelocity(flu2frd(s.vel));
        quad.setPosition(flu2frd(s.pos));
        quad.setAngularAcceleration(flu2frd(s.ang_acc));
        quad.setAngularVelocity(flu2frd(s.ang_vel));
        quad.setQuaternionAttitude(flu2frd(s.quat));
        quad.setMotorOmega(s.omega);
        fse += (frd2flu(quad.getForce()) - s.fmeas).array().square();
        tse += (frd2flu(quad.getTorque()) - s.tmeas).array().square();
    }
    fmse = fse / data.size();
    tmse = tse / data.size();
}

static double loss(Quadcopter &quad, const std::vector<Sample> &data,
                   const std::map<std::string, double> &config, double sf,
                   double st, bool joint)
{
    quad.load(config);
    Array3d fm, tm;
    mse(quad, data, fm, tm);
    double l = fm.sum() / (sf * sf);
    if (joint)
        l += tm.sum() / (st * st);
    return l;
}

static void report(Quadcopter &quad, const std::vector<Sample> &data,
                   const std::map<std::string, double> &config)
{
    quad.load(config);
    Array3d fm, tm;
    mse(quad, data, fm, tm);
    writeYaml(std::cout, config);
    printf("           xy       z        total\n");
    printf("force  %8.4f %8.4f %8.4f  [N]\n", std::sqrt((fm[0] + fm[1]) / 2),
           std::sqrt(fm[2]), std::sqrt(fm.sum() / 3));
    printf("torque %8.5f %8.5f %8.5f  [Nm]\n", std::sqrt((tm[0] + tm[1]) / 2),
           std::sqrt(tm[2]), std::sqrt(tm.sum() / 3));
}

// ---- generic (N,1)-CMA-ES over an unbounded x-space -----------------------
struct Offspring
{
    VectorXd normal; // z ~ N(0, I)
    VectorXd step;   // correlated step B*D*z in the covariance geometry
    VectorXd point;  // candidate x = mean + sigma*step
};

class Cma
{
public:
    explicit Cma(int dim) : dim_(dim), rng_(0), randn_(0.0, 1.0)
    {
        numOffspring_ = 4 + (int)(3 * std::log((double)dim_));
        numParents_ = numOffspring_ / 2;

        weights_.resize(numParents_);
        for (int i = 0; i < numParents_; ++i)
            weights_[i] = std::log(numParents_ + 0.5) - std::log(i + 1.0);
        weights_ /= weights_.sum();
        effectiveParents_ = 1.0 / weights_.squaredNorm();

        pathCovarianceRate_ = (4 + effectiveParents_ / dim_) /
                              (dim_ + 4 + 2 * effectiveParents_ / dim_);
        pathSigmaRate_ =
            (effectiveParents_ + 2) / (dim_ + effectiveParents_ + 5);
        rankOneRate_ = 2.0 / ((dim_ + 1.3) * (dim_ + 1.3) + effectiveParents_);
        rankMuRate_ = std::min(
            1 - rankOneRate_,
            2 * (effectiveParents_ - 2 + 1 / effectiveParents_) /
                ((dim_ + 2) * (dim_ + 2) + effectiveParents_));
        sigmaDamping_ =
            1 +
            2 * std::max(
                    0.0,
                    std::sqrt((effectiveParents_ - 1.0) / (dim_ + 1)) - 1) +
            pathSigmaRate_;
        expectedStepNorm_ =
            std::sqrt((double)dim_) *
            (1 - 1.0 / (4 * dim_) + 1.0 / (21.0 * dim_ * dim_));

        mean_ = VectorXd::Zero(dim_);
        sigma_ = 0.2;
        covariance_ = MatrixXd::Identity(dim_, dim_);
        pathSigma_ = VectorXd::Zero(dim_);
        pathCovariance_ = VectorXd::Zero(dim_);
    }

    double sigma() const { return sigma_; }
    const VectorXd &mean() const { return mean_; }

    std::vector<Offspring> sample()
    {
        Eigen::SelfAdjointEigenSolver<MatrixXd> es(covariance_);
        eigenvectors_ = es.eigenvectors();
        VectorXd axisLengths = es.eigenvalues().cwiseMax(1e-12).cwiseSqrt();

        std::vector<Offspring> population(numOffspring_);
        for (Offspring &o : population)
        {
            o.normal.resize(dim_);
            for (int i = 0; i < dim_; ++i)
                o.normal[i] = randn_(rng_);
            o.step = eigenvectors_ * (axisLengths.asDiagonal() * o.normal);
            o.point = mean_ + sigma_ * o.step;
        }
        return population;
    }

    // `ranked` holds population indices sorted best-first by external fitness.
    void update(const std::vector<Offspring> &population,
                const std::vector<int> &ranked, int gen)
    {
        VectorXd meanNormal = VectorXd::Zero(dim_);
        VectorXd meanStep = VectorXd::Zero(dim_);
        mean_.setZero();
        for (int i = 0; i < numParents_; ++i)
        {
            const Offspring &parent = population[ranked[i]];
            meanNormal += weights_[i] * parent.normal;
            meanStep += weights_[i] * parent.step;
            mean_ += weights_[i] * parent.point;
        }

        pathSigma_ = (1 - pathSigmaRate_) * pathSigma_ +
                     std::sqrt(pathSigmaRate_ * (2 - pathSigmaRate_) *
                               effectiveParents_) *
                         (eigenvectors_ * meanNormal);

        double normalizedPathLength =
            pathSigma_.norm() /
            std::sqrt(1 - std::pow(1 - pathSigmaRate_, 2.0 * (gen + 1))) /
            expectedStepNorm_;
        double heaviside =
            normalizedPathLength < 1.4 + 2.0 / (dim_ + 1) ? 1.0 : 0.0;

        pathCovariance_ =
            (1 - pathCovarianceRate_) * pathCovariance_ +
            heaviside *
                std::sqrt(pathCovarianceRate_ * (2 - pathCovarianceRate_) *
                          effectiveParents_) *
                meanStep;

        MatrixXd rankMuUpdate = MatrixXd::Zero(dim_, dim_);
        for (int i = 0; i < numParents_; ++i)
            rankMuUpdate += weights_[i] * population[ranked[i]].step *
                            population[ranked[i]].step.transpose();

        double stallCorrection =
            (1 - heaviside) * pathCovarianceRate_ * (2 - pathCovarianceRate_);
        covariance_ =
            (1 - rankOneRate_ - rankMuRate_) * covariance_ +
            rankOneRate_ * (pathCovariance_ * pathCovariance_.transpose() +
                            stallCorrection * covariance_) +
            rankMuRate_ * rankMuUpdate;
        covariance_ = (covariance_ + covariance_.transpose()) / 2;

        sigma_ *= std::exp((pathSigmaRate_ / sigmaDamping_) *
                           (pathSigma_.norm() / expectedStepNorm_ - 1));
    }

private:
    int dim_, numOffspring_, numParents_;
    VectorXd weights_;
    double effectiveParents_, pathCovarianceRate_, pathSigmaRate_, rankOneRate_,
        rankMuRate_, sigmaDamping_, expectedStepNorm_;
    VectorXd mean_, pathSigma_, pathCovariance_;
    double sigma_;
    MatrixXd covariance_, eigenvectors_;
    std::mt19937 rng_;
    std::normal_distribution<double> randn_;
};

// x-space bounds/penalty for the box constraint (soft, quadratic)
static constexpr double SIGMA_STOP = 1e-4;
static constexpr double PENALTY = 1e3;

// Maps CMA-ES's unbounded x-space onto the bounded BEM params of the first N
// registry entries (x=0 -> default, |x|=1 -> a bound), with a soft box.
class SearchSpace
{
public:
    explicit SearchSpace(int n) : n_(n), def_(n), scale_(n), xlo_(n), xhi_(n)
    {
        for (int i = 0; i < n_; ++i)
        {
            def_[i] = REGISTRY[i].def;
            scale_[i] = (REGISTRY[i].hi - REGISTRY[i].lo) / 2;
            xlo_[i] = (REGISTRY[i].lo - def_[i]) / scale_[i];
            xhi_[i] = (REGISTRY[i].hi - def_[i]) / scale_[i];
        }
    }

    VectorXd clamp(const VectorXd &x) const
    {
        return x.cwiseMax(xlo_).cwiseMin(xhi_);
    }
    double penalty(const VectorXd &x) const
    {
        return (x - clamp(x)).squaredNorm();
    }
    VectorXd params(const VectorXd &x) const
    {
        return def_ + x.cwiseProduct(scale_);
    }
    std::map<std::string, double> toConfig(const VectorXd &x) const
    {
        std::map<std::string, double> c = defaults();
        VectorXd p = params(x);
        for (int i = 0; i < n_; ++i)
            c[REGISTRY[i].key] = p[i];
        return c;
    }

private:
    int n_;
    VectorXd def_, scale_, xlo_, xhi_;
};

// Opens outdir/convergence_<timestamp>.csv with a "gen,loss,<param...>" header.
static std::ofstream openLog(const std::string &outdir, int N)
{
    char ts[32];
    std::time_t now = std::time(nullptr);
    std::strftime(ts, sizeof(ts), "%Y-%m-%d-%H-%M-%S", std::localtime(&now));
    std::string path = outdir + "/convergence_" + ts + ".csv";
    printf("logging to %s\n", path.c_str());
    std::ofstream log(path);
    log << "gen,loss";
    for (int i = 0; i < N; ++i)
        log << ',' << REGISTRY[i].key;
    log << '\n';
    return log;
}

static double objective(Quadcopter &quad, const std::vector<Sample> &data,
                        const SearchSpace &space, const VectorXd &x, double sf,
                        double st, bool joint)
{
    double value = loss(quad, data, space.toConfig(space.clamp(x)), sf, st,
                        joint) +
                   PENALTY * space.penalty(x);
    return std::isfinite(value) ? value : 1e12;
}

static std::vector<int> rankByFitness(const std::vector<double> &fitness)
{
    std::vector<int> order(fitness.size());
    for (size_t i = 0; i < order.size(); ++i)
        order[i] = (int)i;
    std::sort(order.begin(), order.end(),
              [&](int a, int b)
              { return fitness[a] < fitness[b]; });
    return order;
}

static void logGeneration(std::ofstream &log, int gen, double loss,
                          const VectorXd &params)
{
    log << gen << ',' << loss;
    for (Eigen::Index i = 0; i < params.size(); ++i)
        log << ',' << params[i];
    log << '\n';
    log.flush();
}

static std::map<std::string, double> cma(Quadcopter &quad,
                                         const std::vector<Sample> &data, int N,
                                         bool joint, double sf, double st,
                                         const std::string &outdir)
{
    SearchSpace space(N);
    Cma optimizer(N);
    std::ofstream log = openLog(outdir, N);

    VectorXd best_x = optimizer.mean();
    double best_loss = 1e18;

    for (int gen = 0; gen < MAXGEN; ++gen)
    {
        std::vector<Offspring> population = optimizer.sample();

        std::vector<double> fitness(population.size());
        for (size_t k = 0; k < population.size(); ++k)
            fitness[k] = objective(quad, data, space, population[k].point, sf,
                                   st, joint);

        std::vector<int> ranked = rankByFitness(fitness);
        optimizer.update(population, ranked, gen);

        int fittest = ranked[0];
        if (fitness[fittest] < best_loss)
        {
            best_loss = fitness[fittest];
            best_x = space.clamp(population[fittest].point);
        }

        logGeneration(log, gen, best_loss, space.params(best_x));
        if (gen % 10 == 0)
            printf("gen %3d  loss %.5f  sigma %.4f\n", gen, best_loss,
                   optimizer.sigma());
        if (optimizer.sigma() < SIGMA_STOP)
            break;
    }
    return space.toConfig(best_x);
}

struct Args
{
    const char *data = nullptr;
    const char *cfg = nullptr;
    int N = 0;
    bool do_cma = false, joint = false;
};

static bool parseArgs(int argc, char **argv, Args &a)
{
    if (argc < 2)
        return false;
    a.data = argv[1];
    for (int i = 2; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--cma" && i + 1 < argc)
        {
            a.do_cma = true;
            a.N = std::stoi(argv[++i]);
        }
        else if (arg == "--joint")
            a.joint = true;
        else if (!a.cfg && !arg.empty() && arg[0] != '-')
            a.cfg = argv[i];
        else
            return false;
    }
    if (a.do_cma && (a.cfg || a.N < 1 || a.N > (int)REGISTRY.size()))
        return false;
    return true;
}

int main(int argc, char **argv)
{
    Args args;
    if (!parseArgs(argc, argv, args))
    {
        fprintf(stderr,
                "usage: %s data.csv [config.yaml] | data.csv --cma N [--joint]\n",
                argv[0]);
        return 1;
    }

    std::vector<Sample> data = load(args.data);
    Quadcopter quad;

    double sf2 = 0, st2 = 0;
    for (const Sample &s : data)
    {
        sf2 += s.fmeas.squaredNorm();
        st2 += s.tmeas.squaredNorm();
    }
    double sf = std::sqrt(sf2 / data.size()), st = std::sqrt(st2 / data.size());

    if (args.do_cma)
    {
        printf("loaded %zu rows | sf=%.3f st=%.4f | %d params free | %s\n",
               data.size(), sf, st, args.N,
               args.joint ? "force+torque" : "force-only");
        printf("--- baseline (defaults) ---\n");
        report(quad, data, defaults());
        printf("--- CMA-ES ---\n");
        std::string outdir =
            (std::filesystem::path(args.data).parent_path().parent_path() /
             "CMAES-results")
                .string();
        std::map<std::string, double> best =
            cma(quad, data, args.N, args.joint, sf, st, outdir);
        std::ofstream out("best.yaml");
        writeYaml(out, best);
        printf("--- best (written to best.yaml) ---\n");
        report(quad, data, best);
    }
    else
    {
        try
        {
            report(quad, data, args.cfg ? loadConfig(args.cfg) : defaults());
        }
        catch (const std::out_of_range &)
        {
            printf("Config incomplete: a required key is missing\n");
            return 1;
        }
    }
    return 0;
}