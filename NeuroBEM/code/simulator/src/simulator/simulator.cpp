#include "simulator.h"

/* Constructor for the simulator. This version is used if the class is
 * created without arguments and mostly meant for testing purposes. */
Simulator::Simulator() {
  Propeller p(true);
  p.setOmega(2000);
  p.setVelocity(Eigen::Vector3d{10, 0, -2});
  p.setAttitudeRate(Eigen::Vector3d{0, 0, 0});
  p.printInfo();
}

/* Constructor for the simulator. This one is used to evaluate the simulation
 * for a csv file, which should be created with the MergeAndProcess MATLAB
 * script. */
Simulator::Simulator(const char* infile, const char* outfile,
                     const std::map<std::string, double>& config) {
  csvReader csv(infile);
  size_t nCol = csv.getNumberOfColumns();
  size_t nRow = csv.getNumberOfRows();

  Quadcopter quad;
  if (!config.empty()) {
    try {
      quad.load(config);
    } catch (const std::out_of_range&) {
      printf("Config incomplete: a required key is missing\n");
      exit(1);
    }
  }
  std::string out(outfile);
  size_t slash = out.find_last_of('/');
  quad.log_params((slash == std::string::npos ? "" : out.substr(0, slash + 1)) +
                  "params.yaml");

  std::vector<double> data(nCol + 6, 0);

  Eigen::Vector3d ang_acc;
  Eigen::Vector3d ang_vel;
  Eigen::Quaterniond quat;
  Eigen::Vector3d acc;
  Eigen::Vector3d vel;
  Eigen::Vector3d pos;
  Eigen::Vector3d force;
  Eigen::Vector3d torque;
  std::vector<double> omega(4, 0);
  std::vector<double> domega(4, 0);

  csvWriter writer(outfile, nCol + 6);

  Runtime_s rt;
  for (size_t i = 0; i < nRow; ++i) {
    rt.start();
    csv.getLine(&data, i);
    memcpy(ang_acc.data(), data.data() + 1, 3 * sizeof(double));
    memcpy(ang_vel.data(), data.data() + 4, 3 * sizeof(double));
    memcpy(&quat.coeffs(), data.data() + 7, 4 * sizeof(double));
    memcpy(acc.data(), data.data() + 11, 3 * sizeof(double));
    memcpy(vel.data(), data.data() + 14, 3 * sizeof(double));
    memcpy(pos.data(), data.data() + 17, 3 * sizeof(double));
    memcpy(omega.data(), data.data() + 20, 4 * sizeof(double));
    memcpy(domega.data(), data.data() + 24, 4 * sizeof(double));
    quad.setAcceleration(flu2frd(acc));
    quad.setVelocity(flu2frd(vel));
    quad.setPosition(flu2frd(pos));
    quad.setAngularAcceleration(flu2frd(ang_acc));
    quad.setAngularVelocity(flu2frd(ang_vel));
    quad.setQuaternionAttitude(flu2frd(quat));
    quad.setMotorOmega(omega);
    force = frd2flu(quad.getForce());
    torque = frd2flu(quad.getTorque());
    memcpy(data.data() + nCol, force.data(), 3 * sizeof(double));
    memcpy(data.data() + nCol + 3, torque.data(), 3 * sizeof(double));
    writer.add(data);
    rt.stop();
  }
  writer.close();
  rt.print("Avg. BEM Runtime [µs]");
}
