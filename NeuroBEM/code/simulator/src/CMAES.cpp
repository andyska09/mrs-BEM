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
    double def, lo, hi; // defaults must match params.h; pitch/twist in degrees
};

// ordered by optimization priority: --cma N frees the first N entries
static const std::vector<ParamSpec> REGISTRY = {
    {"lift_coefficient", 15.24214, 0.5, 40},
    {"drag_coefficient", 13.54894, 0.5, 40},
    {"hinge_spring_constant", 5.89, 0.5, 30},
    {"lift_offset", 0, -0.2, 0.3},
    {"hforce_scale", 1, 0.1, 6},
    {"thrust_scale", 1, 0.5, 2},
    {"pitch", 21.77, 3, 40},
    {"twist", -11, -34, 6},
    {"chord_inner", 1.7e-2, 0.005, 0.04},
    {"chord_outer", 0.7e-2, 0.002, 0.03},
    {"horizontal_drag_coefficient", 1, 0.1, 5},
    {"vertical_drag_coefficient", 1, 0.1, 5},
    {"radius", 0.064770, 0.04, 0.09},
    {"dx", 0.078, 0.04, 0.15},
    {"dy", 0.1, 0.05, 0.2},
    {"dz", 0.027, -0.05, 0.1},
    {"frontarea_x", 0.06 * 0.09, 0, 0.03},
    {"frontarea_y", 0.1 * 0.09, 0, 0.03},
    {"frontarea_z", 0.1 * 0.06, 0, 0.03},
    // load-only tail: keep N below these so they stay at default
    {"num_blades", 3, 2, 4},
    {"air_density", 1.204, 1.0, 1.4},
};

static const std::map<std::string, std::vector<const char *>> SECTIONS = {
    {"bem",
     {"lift_coefficient", "drag_coefficient", "hinge_spring_constant", "pitch",
      "twist", "chord_inner", "chord_outer", "radius", "lift_offset",
      "hforce_scale", "num_blades", "air_density"}},
    {"quad", {"thrust_scale", "dx", "dy", "dz"}},
    {"body_drag",
     {"horizontal_drag_coefficient", "vertical_drag_coefficient", "frontarea_x",
      "frontarea_y", "frontarea_z"}},
};

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
    for (const auto &[section, keys] : SECTIONS)
    {
        os << section << ":\n";
        for (const char *key : keys)
            os << "  " << key << ": " << c.at(key) << '\n';
    }
}

struct Sample
{
    Vector3d ang_acc, ang_vel, acc, vel, pos, fmeas, tmeas;
    Eigen::Quaterniond quat;
    std::vector<double> omega;
};

static std::vector<Sample> load(const std::string &path)
{
    std::vector<Sample> out;
    std::ifstream file(path);
    if (!file)
    {
        fprintf(stderr, "cannot open %s\n", path.c_str());
        exit(1);
    }
    std::string line, cell;
    while (std::getline(file, line))
    {
        if (!line.empty() && !std::isdigit(line[0]) && line[0] != '-')
            continue;
        std::stringstream ss(line);
        std::vector<double> v;
        while (std::getline(ss, cell, ','))
            v.push_back(std::stod(cell));
        if (v.size() < 28)
            continue;
        Sample s;
        s.ang_acc = {v[1], v[2], v[3]};
        s.ang_vel = {v[4], v[5], v[6]};
        s.quat = Eigen::Quaterniond(v[10], v[7], v[8], v[9]); // w, x, y, z
        s.acc = {v[11], v[12], v[13]};
        s.vel = {v[14], v[15], v[16]};
        s.pos = {v[17], v[18], v[19]};
        s.omega = {v[20], v[21], v[22], v[23]};
        s.fmeas = MASS * s.acc;
        s.tmeas = INERTIA.cwiseProduct(s.ang_acc) +
                  s.ang_vel.cross(INERTIA.cwiseProduct(s.ang_vel));
        out.push_back(s);
    }
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
    Array3d fr = fm.sqrt(), tr = tm.sqrt();
    writeYaml(std::cout, config);
    printf("force  RMSE: %.4f %.4f %.4f\n", fr[0], fr[1], fr[2]);
    printf("torque RMSE: %.5f %.5f %.5f\n", tr[0], tr[1], tr[2]);
}

static std::map<std::string, double> cma(Quadcopter &quad,
                                         const std::vector<Sample> &data, int N,
                                         bool joint, double sf, double st,
                                         const std::string &outdir)
{
    VectorXd def(N), scale(N), xlo(N), xhi(N);
    for (int i = 0; i < N; ++i)
    {
        def[i] = REGISTRY[i].def;
        scale[i] = (REGISTRY[i].hi - REGISTRY[i].lo) / 2;
        xlo[i] = (REGISTRY[i].lo - def[i]) / scale[i];
        xhi[i] = (REGISTRY[i].hi - def[i]) / scale[i];
    }
    auto toConfig = [&](const VectorXd &x)
    {
        std::map<std::string, double> c = defaults();
        for (int i = 0; i < N; ++i)
            c[REGISTRY[i].key] = def[i] + x[i] * scale[i];
        return c;
    };

    const int lambda = 4 + (int)(3 * std::log((double)N));
    const int mu = lambda / 2;
    VectorXd w(mu);
    for (int i = 0; i < mu; ++i)
        w[i] = std::log(mu + 0.5) - std::log(i + 1.0);
    w /= w.sum();
    const double mueff = 1.0 / w.squaredNorm();
    const double cc = (4 + mueff / N) / (N + 4 + 2 * mueff / N);
    const double cs = (mueff + 2) / (N + mueff + 5);
    const double c1 = 2.0 / ((N + 1.3) * (N + 1.3) + mueff);
    const double cmu = std::min(
        1 - c1, 2 * (mueff - 2 + 1 / mueff) / ((N + 2) * (N + 2) + mueff));
    const double ds =
        1 + 2 * std::max(0.0, std::sqrt((mueff - 1.0) / (N + 1)) - 1) + cs;
    const double chiN =
        std::sqrt((double)N) * (1 - 1.0 / (4 * N) + 1.0 / (21.0 * N * N));

    VectorXd mean = VectorXd::Zero(N); // x-coords: param = def + x * scale
    double sigma = 0.2;
    MatrixXd C = MatrixXd::Identity(N, N);
    VectorXd ps = VectorXd::Zero(N), pc = VectorXd::Zero(N);
    std::mt19937 rng(0);
    std::normal_distribution<double> randn(0.0, 1.0);

    VectorXd best_x = mean;
    double best_f = 1e18;
    char ts[32];
    std::time_t now = std::time(nullptr);
    std::strftime(ts, sizeof(ts), "%Y-%m-%d-%H-%M-%S", std::localtime(&now));
    std::string logpath = outdir + "/convergence_" + ts + ".csv";
    printf("logging to %s\n", logpath.c_str());
    std::ofstream log(logpath);
    log << "gen,loss";
    for (int i = 0; i < N; ++i)
        log << ',' << REGISTRY[i].key;
    log << '\n';

    for (int gen = 0; gen < MAXGEN; ++gen)
    {
        Eigen::SelfAdjointEigenSolver<MatrixXd> es(C);
        MatrixXd B = es.eigenvectors();
        VectorXd D = es.eigenvalues().cwiseMax(1e-12).cwiseSqrt();

        std::vector<VectorXd> zs(lambda), ys(lambda), xs(lambda);
        std::vector<std::pair<double, int>> fit(lambda);
        for (int k = 0; k < lambda; ++k)
        {
            VectorXd z(N);
            for (int i = 0; i < N; ++i)
                z[i] = randn(rng);
            VectorXd y = B * (D.asDiagonal() * z);
            VectorXd x = mean + sigma * y;
            VectorXd xc = x.cwiseMax(xlo).cwiseMin(xhi);
            double pen = (x - xc).squaredNorm();
            double f = loss(quad, data, toConfig(xc), sf, st, joint) + 1e3 * pen;
            if (!std::isfinite(f))
                f = 1e12;
            zs[k] = z;
            ys[k] = y;
            xs[k] = x;
            fit[k] = {f, k};
        }
        std::sort(fit.begin(), fit.end());

        VectorXd zmean = VectorXd::Zero(N), ymean = VectorXd::Zero(N);
        mean.setZero();
        for (int i = 0; i < mu; ++i)
        {
            int k = fit[i].second;
            zmean += w[i] * zs[k];
            ymean += w[i] * ys[k];
            mean += w[i] * xs[k];
        }

        ps = (1 - cs) * ps + std::sqrt(cs * (2 - cs) * mueff) * (B * zmean);
        double hsig =
            ps.norm() / std::sqrt(1 - std::pow(1 - cs, 2.0 * (gen + 1))) / chiN <
                    1.4 + 2.0 / (N + 1)
                ? 1.0
                : 0.0;
        pc = (1 - cc) * pc + hsig * std::sqrt(cc * (2 - cc) * mueff) * ymean;

        MatrixXd rank_mu = MatrixXd::Zero(N, N);
        for (int i = 0; i < mu; ++i)
            rank_mu += w[i] * ys[fit[i].second] * ys[fit[i].second].transpose();
        C = (1 - c1 - cmu) * C +
            c1 * (pc * pc.transpose() + (1 - hsig) * cc * (2 - cc) * C) +
            cmu * rank_mu;
        C = (C + C.transpose()) / 2;
        sigma *= std::exp((cs / ds) * (ps.norm() / chiN - 1));

        if (fit[0].first < best_f)
        {
            best_f = fit[0].first;
            best_x = xs[fit[0].second].cwiseMax(xlo).cwiseMin(xhi);
        }
        log << gen << ',' << best_f;
        for (int i = 0; i < N; ++i)
            log << ',' << def[i] + best_x[i] * scale[i];
        log << '\n';
        log.flush();
        if (gen % 10 == 0)
            printf("gen %3d  loss %.5f  sigma %.4f\n", gen, best_f, sigma);
        if (sigma < 1e-4)
            break;
    }
    log.close();
    return toConfig(best_x);
}

int main(int argc, char **argv)
{
    bool do_cma = false, joint = false, bad = false;
    int N = 0;
    const char *cfg = nullptr;
    for (int i = 2; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--cma" && i + 1 < argc)
            do_cma = true, N = std::stoi(argv[++i]);
        else if (arg == "--joint")
            joint = true;
        else if (!cfg && arg[0] != '-')
            cfg = argv[i];
        else
            bad = true;
    }
    if (argc < 2 || bad || (do_cma && (cfg || N < 1 || N > (int)REGISTRY.size())))
    {
        fprintf(stderr,
                "usage: %s data.csv [config.yaml] | data.csv --cma N [--joint]\n",
                argv[0]);
        return 1;
    }

    std::vector<Sample> data = load(argv[1]);
    Quadcopter quad;

    double sf2 = 0, st2 = 0;
    for (const Sample &s : data)
    {
        sf2 += s.fmeas.squaredNorm();
        st2 += s.tmeas.squaredNorm();
    }
    double sf = std::sqrt(sf2 / data.size()), st = std::sqrt(st2 / data.size());

    if (do_cma)
    {
        printf("loaded %zu rows | sf=%.3f st=%.4f | %d params free | %s\n",
               data.size(), sf, st, N, joint ? "force+torque" : "force-only");
        printf("--- baseline (defaults) ---\n");
        report(quad, data, defaults());
        printf("--- CMA-ES ---\n");
        std::string outdir =
            (std::filesystem::path(argv[1]).parent_path().parent_path() /
             "CMAES-results")
                .string();
        std::map<std::string, double> best =
            cma(quad, data, N, joint, sf, st, outdir);
        std::ofstream out("best.yaml");
        writeYaml(out, best);
        printf("--- best (written to best.yaml) ---\n");
        report(quad, data, best);
    }
    else
    {
        try
        {
            report(quad, data, cfg ? loadConfig(cfg) : defaults());
        }
        catch (const std::out_of_range &)
        {
            printf("Config incomplete: a required key is missing\n");
            return 1;
        }
    }
    return 0;
}
