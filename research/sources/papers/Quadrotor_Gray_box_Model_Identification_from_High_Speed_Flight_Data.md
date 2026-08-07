Quadrotor Gray-box Model Identification from High-Speed
Flight Data
Sihao Sun∗ , Coen C. de Visser† and Qiping Chu‡
Delft University of Technology, 2629 HS Delft, The Netherlands

In order to explore the aerodynamic effects on a quadrotor in the high-speed flight regime
and establish an accurate nonlinear model, free flight tests with a quadrotor have been carried
out in a large-scale wind tunnel. The flight data reveal that complex aerodynamic interactions
could appear and significantly influence the forces and moments acting on the quadrotor, which
indicates the inaccuracy of state-of-art models established based on helicopter aerodynamic
theory. To cope with this problem, gray-box models considering these effects are identified
from flight data using a stepwise system identification approach, which combines both priorknowledge of rotorcraft aerodynamic properties as well as data observations. Previous models
introduced in the literature are compared with the gray-box models. Validation results show
an 80% reduction of moment model residuals and 20% reduction of force model residuals.

Nomenclatures
α, β

=

angle of attack and sideslip angle, rad

ρ

=

air density, kg/m3

g

=

acceleration of gravity, m/s2

V , V g , V wind

=

airspeed, ground speed and wind speed, m/s.

u, v, w

=

airspeed components in the body frame, m/s

Ω, p, q, r

=

body angular rates, rad/s

I v , Ix , Iy , Iz , Ixz

=

inertia moment of the vehicle, kg·m2

Ip

=

inertia moment of the propeller, kg·m2

m

=

total mass of the vehicle, kg

b, l, R

=

vehicle geometry parameters, rotor radius, m

κ0 , τ0

=

force and torque coefficient of the hovering model, N·s, N·m·s

λr

=

damping rate in the hovering model, N·m·s

R BG

=

rotational matrix from the ground frame to the body frame

∗ Ph.D Student, Control and Simulation Section, Faculty of Aerospace Engineering, Kluyverweg 1.
† Assistant Professor, Control and Simulation Section, Faculty of Aerospace Engineering, Kluyverweg 1. Member AIAA.
‡ Associate Professor, Control and Simulation Section, Faculty of Aerospace Engineering, Kluyverweg 1.

F, Fx , Fy , Fz

=

aerodynamic forces, N

M, Mx , My , Mz

=

aerodynamic moments, N·m

Mh, Fh

=

aerodynamic force and moment estimation from the hovering model, N and N·m respectively

M Ip

=

rotor inertia related moment

T

=

thrust, N

Ct

=

thrust coefficient

Cx , Cy , Cz ,Cl , Cm , Cn

=

aerodynamic forces and moments coefficients

Ch

=

horizontal force coefficient

Ωi

=

rotation speed of the ith rotor, rad/s

Ω̄

=

geometric average of rotor speeds, rad/s

ωi

=

normalized rotation speed of the ith rotor

u p , uq , ur

=

control inputs for roll, pitch and yaw

N

=

number of rotors, or, number of measurement samples

µ x , µ y , µz

=

advance ratios

µh

=

horizontal advanced ratio

p̄, q̄, r̄

=

normalized body angular rates

z

=

force and moment measurements

y

=

model outputs



=

model residual

A

=

regressor matrix

θ, θ̂

=

vector of model parameters and its estimation

S(·)

=

candidate set

x

=

vector of model independent variables

P d (x)

=

set comprising all basis of the dth order polynomial with x as the independent variable

n

=

dimension of x

dˆ

=

number of terms in P d (x)

ξ

=

regressor

ν̄in

=

normalized induced velocity

σ

=

standard deviation

2

I. Introduction
Multi rotor drones are widely used currently as an efficient tool in multiple applications such as reconnaissance,
package delivery, agriculture monitoring, filming and even personal transportation. Multi-rotor drones are equipped
with individual rotors producing both propulsion and control power and frequently operate in non-hovering conditions in
out-door environments. During flights with non-static incoming flow in these conditions, the aerodynamic characteristics
of these rotors are different from those modeled in static conditions and considerable free-stream induced aerodynamic
effects become apparent.
Drones are able to operate in conditions where additional aerodynamic effects occur without full knowledge of them
due to the high update rates of sensors and robustness of the controller [1–3]. However, knowledge of these effects
will be necessary for controllers capable of fully exploring the flight envelope [4, 5] such as high speed flights with
aggressive maneuveres. Next to the controller enhancement, the modeling of these aerodynamic effects is also desirable
of providing better attitude estimation [6] and refining the design process [7]. In addition, full knowledge of these
aerodynamic effects is also required for high fidelity simulation platforms [8] and finally, global models need to be
established for flight envelope computation [9], which is the main motivation for the current work.
The main subject of this research is the quadrotor, one of the simplest possible multi rotor drones. The aerodynamic
effects acting on quadrotors can be summarized as the force variation and moment variation compared to that in the
hovering condition without ground effect. Several discussions about these forces and moment variations are present in
the literature.
Most literature sources focus on improving the thrust model. Refs. [10] , for instance, elaborate the cause of thrust
variation during translational flight. The modeling process is mostly derived from helicopter aerodynamic theories.
Refs. [4, 10, 11] use momentum theory to develop the model of relationships between thrust efficiency, flight speed,
and the angle of attack. Thrust calculation of a single rotor according to blade element theory is adopted [12–15].
Momentum theory and blade element theory are also combined and a so-called blade element momentum (BEMT)
theory is used to enhance the thrust model accuracy [16–18] .
Drag forces, which are mostly defined in the blade plane of multi-rotor drones, are also discussed in literature. The
blade flapping effect is considered the main cause of drag force [10, 15, 19–21]. According to Ref. [12, 17, 19, 20], lift
also induces an aerodynamic drag on the blade elements and generates a hub force perpendicular to the thrust. Besides
the resistance caused by the rotor, the aerodynamic drag from the airframe is also considered [16], which is quadratic
related to the flight speed.
Compared to forces, moment variations have received less attention in existing literature. The additional pitching
moment due to the translational velocities is observed in the wind tunnel test presented in Ref. [22]. In the trim condition
during forward flight, the aft rotors rotate faster than the front rotor. Damping effects [21], blade stiffness [11] and
drag forces [21] are considered to cause these moments as well. The bare airframe itself may also generate a pitching
3

moment [8]. To the best of the author’s knowledge no high fidelity models of the aerodynamic moments exist in the
literature.
Besides the forces and moments generated from individual propellers and airframe, rotor-rotor and airframe-rotor
interaction effects are suspected to greatly influence the aerodynamic forces and moments. Ref. [13] shows that the
interaction between multiple rotors deteriorates the total thrust from wind tunnel tests. Ref. [8] divides the forces
and moments into propulsion, airframe and interaction units, and the research of interaction terms is still ongoing.
Models considering the interaction effect have been made [7][23] based on physical theory and engineering assumptions,
however, not validated with in-flight data. The actual effects of these interactions on thrust, drag and moments remain
largely unknown, which should be investigated with free flight experiments.
The main contribution of this research is further revealing the effect of above interactions from flight data and
establish accurate force and moment models taking account of these effects. To this end, multiple free flight tests have
been carried out. Based on the data from these tests, it is shown that the interaction effects deteriorate the well-established
thrust and drag model based on the first principles, and in addition demonstrate significant inaccuracies in the pitch and
rolling moment predictions obtained with the broadly accepted quadrotor hovering model, i.e the distance of propellers
times their thrust differences. Furthermore, it is shown that the yawing moment is strongly influenced by the incoming
flow during high speed flight, which has not been discussed before.
To establish a high fidelity model of forces and moments which is valid in a larger flight envelope, a system
identification approach is used. Different from the first-principles modeling approach derived from helicopter
aerodynamic theory, system identification methods are proper choice for modeling these complex interaction effects.
Specifically, a gray-box model is established which combines the information from prior-physical knowledge of
rotor-craft theory with experimental data obtained during high speed flight, and possesses both reliability of physical
theory and accuracy of observation.
Several system identification techniques can be applied to establish nonlinear gray-box models, depending on the
structure of the model, such as polynomial functions, multivariate spline-functions, neural networks, etc. Among
them, a simple but effective piecewise polynomial structure is selected. A stepwise method is used for determining the
model structure by selecting terms from a large set of candidate terms. This technique has been used in the past for
full-scale aircraft system identification [24–26] but has never been seen used for determining the aerodynamic model of
a quadrotor. The model structure candidates are determined from prior-knowledge of rotor-craft aerodynamic theories
as well as preliminary assumptions. The stepwise method selects candidates into the model in a stepwise scheme
according to their contributions to the current model.
The identified models are compared with state-of-art force models considering aerodynamic effects as well as
moment models established in hovering conditions. The validation results reveal around 20% improvement in the
accuracy of force model and more importantly, over 80% improvement in the moment model in terms of the residual
4

root mean square (RMS) in non-hovering conditions. Although these models are specific to the Bebop platform, the
methodologies can be generalized to other multi-rotor platforms.
The flight experiments are carried out in the Open Jet Facility (OJF), a large scale wind tunnel with a 3 meters
aperture operated by the Delft University of Technology, as shown in Fig. 1. In contrast to static wind tunnel tests,
free flights are performed in the OJF in order to negate the disturbance effect of a force balance and more importantly,
to take dynamic motions into account. The wind tunnel provides 2.5 m by 2.5 m by 5.0 m space to carry out these
flights. A large number of different flight maneuvers are made to fully excite the system and a maximum air speed up to
14 m/s is achieved. An of-the-shelf quadrotor (Parrot Bebop) running open-source autopilot (Paparazzi) is used in these
flights. The standard build-in inertia measurement unit (IMU) running at 512 Hz and external motion capture systems
(Optitrack) running at 360 Hz are sensor-fused for data acquisition [27].

Fig. 1

Open Jet Facility (OJF), a large-scale wind tunnel, and the tested quadrotor.

A normalization method for modeling multi-rotor drones in terms of dimensionless coefficients is proposed in
this research. The dimensionless aerodynamic coefficients and states are analogous to those used for single rotorcraft.
Moment coefficients are for the first time introduced for quadrotor drones, taking into account their multi-rotor
characteristics. The gray-box model established will be presented in dimensionless form as a mapping between
dimensionless states and force (moment) coefficients. These dimensionless coefficients are also useful for revealing the
interaction effects, and even for comparing the aerodynamic properties of different drone models.
The article is organized as follows. Chapter II introduces coordinate definition and hovering model definition.
Chapter III depicts the stepwise method and provides definition of dimensionless aerodynamic coefficients and other
dimensionless variables related to the model. Chapter IV describes the flight test for this research and discusses the
interaction effects observed from flight data. The identification process and results can be found in Chapter V and VI
respectively.

5

II. Preliminary Modeling
A benchmark model is introduced in this chapter with the aim of further introducing the gray-box model and make
comparisons between them. First, the two coordinate systems, in the form of the ground frame and the body frame, are
defined (Fig. 2). For the ground frame, xG is defined towards the wind tunnel nozzle, in order words, into the free
stream; zG is aligned with the gravity direction pointing downwards. The body frame is fixed to the vehicle with the
center of gravity at the origin. xB is aligned with the nose direction, yB points to the right and z B points against the
thrust direction.
Airspeed, the flight speed with respect to the air stream, is defined as
V = V g − V wind

(1)

where V g and V wind indicate ground speed and wind speed respectively. The projection of airspeed on the body frame
is expressed as V = [u

v

w]T . Angle of attack α and sideslip angle β are defined as
α = arcsin(w/V)

p
β = arcsin(v/ v 2 + u2 )

(2)

where V = ||V ||. Note that since the quadrotor is able to hover and reverse, these two angles are singular when airspeed
equals zero.

Fig. 2

Coordinate systems definition and sketch of Parrot Bebop quadrotor.

Rotor speeds (in rad/s) are expressed as [Ω1

Ω2

Ω3

Ω4 ] respectively. Fig. 2 shows the rotor index and their

rotation directions of Parrot Bebop quadrotor, which is the object to be modeled in this paper.
For simplicity, the quadrotor is regarded as a rigid-body of which the translational and rotational dynamic equations
can be written as
VÛ + Ω × V = R BG g + F/m

6

(3)

Û + Ω × I v Ω = M + M Ip
I vΩ
where g = [0 0

g]T indicates the gravity vector expressed in the ground frame. Ω = [p

(4)
q

r]T represents

the angular velocity expressed in the body frame. The aerodynamic forces and moments are denoted as F and M
respectively, which are expressed in the body frame as well. R BG is the rotational matrix from the ground frame to the
body frame. I v stands for the inertia matrix and m indicates the mass of the vehicle. M I p represents the moments due
to gyroscopic effects and rotor spin-up torque; the later has been found significantly influence the Bebop quadrotor [28]


 qI (−Ω + Ω − Ω + Ω ) 
 p
1
2
3
4 





M I p =  pI p (Ω1 − Ω2 + Ω3 − Ω4 ) 




 I (−Ω
Û 4 ) 
Ω2 − Û
Ω3 + Ω
 p Û1 + Û



(5)

The positioning of a quadrotor in 3D space is controlled by changing its attitude and total thrust. The attitude can
be changed by differential thrust. Specifically, rotor speed difference between front and aft rotors produces a pitching
moment, while a rolling moment can be produced by differential thrust between left and right rotors. The rotor reaction
torque is used to generate a yawing moment which the quadrotor uses to control its heading.
The emphasis of modeling is on the aerodynamic force vector F and the aerodynamic moment vector M. Before
establishing a gray-box model for F and M , a hovering model that is only valid in hovering condition, is introduced as
a benchmark for comparison




0








Fh = 
0





 −κ Í Ω2 

0
i 






bκ0 (Ω21 + Ω22 − Ω23 − Ω24 )







2
2
2
2
M h = 
lκ0 (Ω1 − Ω2 − Ω3 + Ω4 )





 τ (−Ω2 + Ω2 − Ω2 + Ω2 ) + λ r 
 0

r
1
2
3
4



(6)

(7)

where l and b are geometry parameters of the quadrotor as Fig. 2 shows. κ0 , τ0 and λr are constant coefficients. Note
that the aerodynamic forces and moments are expressed in the body frame in this paper. Therefore, the third component
of F h equals to the negative of the total thrust. Meanwhile, the first two components of F h equal zero, which means
that rotor in-plane forces (i.e drag forces) are neglected.
However, significant in-plane drag has been found [6, 12, 17, 19] and thrust also varies with the flight speed beyond
the hovering regime [11]. Furthermore, aerodynamic moments are found to be completely different from what the

7

hovering model predicts. These studies indicate the importance of finding a model which is valid in a larger flight
envelope.
In this paper, a gray-box model is identified from high-speed free-flight data obtained in a wind tunnel. The
aerodynamic effect of individual rotors, the rotor-rotor and rotor-airframe aerodynamic interactions in high-speed
conditions are considered in this gray-box model as well.

III. Methodologies
A. Nondimensionalization
Dimensionless aerodynamic coefficients are convenient for comparisons between different conditions and platforms.
For a single rotor, forces and moments can be normalized by rotor speed and reference area [8]. However, for multi-rotor
aircraft such as quadrotors, determining the aerodynamic coefficient of each rotor could be impracticable using system
identification approach since only joint forces are measurable by the 3-axis accelerometer located at the center of gravity.
Furthermore, the local airspeed differs between rotors because of complex aerodynamic interactions, which makes the
rotor-by-rotor modeling approach impractical.
In this research, a novel nondimensionalization approach is proposed which is based on an assumption that
aerodynamic forces and moments are mainly generated by the rotor system. A geometric average of rotor speeds is used
to represent the effect of multi-rotors
s
Ω̄ =

ÍN

2
i=1 Ωi

(8)

N

where N is the number of rotors and equals 4 for quadrotor. In most cases, rotors are the same size with radius R.
Afterwards, forces and moments acting on the entire vehicle can be normalized by the average rotor speed
Fz
,
ρ(N πR2 )(RΩ̄)2

Cx =

Fx
,
ρ(N πR2 )(RΩ̄)2

Mx
,
ρb(N πR2 )(RΩ̄)2

Cm =

My

Cz =

Cl =

ρb(N πR2 )(RΩ̄)2

Cy =

,

Cn =

Fy
ρ(N πR2 )(RΩ̄)2

Mz
ρb(N πR2 )(RΩ̄)2

(9)

(10)

where b is the reference length chosen arbitrarily as long as it represents the geometry size of a specific vehicle. Since
Fz is opposite to thrust which brings intuitive inconvenience, T = −Fz is used as the total thrust force and Ct = −Cz as
the thrust coefficient. Note that T is interpreted as the joint of rotor thrust and drag force along the body vertical axis.
Translational and angular velocities, which have been found to significantly influence the abovementioned
coefficients [10, 19], are normalized by
µx =

u
,
Ω̄R

µy =
8

v
,
Ω̄R

µz =

w
Ω̄R

(11)

p̄ =

pb
,
Ω̄R

The horizontal component of the advance ratio µ =

q̄ =
q

qb
,
Ω̄R

r̄ =

rb
Ω̄R

µ2x + µ2y + µ2z , which is defined as µh =

(12)
q

µ2x + µ2y , is used to

analyze interaction effects in this research. These dimensionless parameters are analogous to those used for single-rotor
aircraft.
The rotor speeds are normalized by
ωi =

Ωi
Ω̄

(13)

It is assumed that Ω̄ > 0 always holds to avoid the singularity, because the status that all rotors are stopped is out of the
scope of this paper.
The moments for controlling attitude are produced by differential thrust from rotor speeds differences. Here three
normalized inputs for roll, pitch and yaw controls are defined
u p = (ω12 + ω42 ) − (ω22 + ω32 )

(14)

uq = (ω12 + ω22 ) − (ω32 + ω42 )

(15)

ur = −(ω12 + ω32 ) + (ω22 + ω42 )

(16)

where signs and numbers are in accordance with the definition in Fig. 2, which may change for different types of
quadrotor vehicles.

B. Stepwise System Identification
This section introduces the system identification approach applied to establish the gray-box model. Specifically,
a regression method together with a model structure selection algorithm is used for determining mappings from
dimensionless states to aerodynamic force and moment coefficients. The relation of model outputs to measurements
satisfies
z = y +  = Aθ + 

(17)

where z ∈ R N stands for N measured dimensionless forces and moments. y = Aθ denotes model output and  ∈ R N
vector indicates model residuals. A ∈ R N ×p is the regressor matrix with each column as a regressor which is an
arbitrarily combination of independent variables. θ ∈ R p stands for parameters of regressors to be estimated using , e.g
Original Least Square (OLS) estimator
θ̂ = (AT A)−1 AT z

9

(18)

where θ̂ is the optimal parameter estimation that minimizes the sum of squares of the residual  . The process of model
structure selection is concerned with the choice of particular regressors in the A matrix.
In this paper, two steps are taken in the model structure selection. The first step is defining a candidate regressors set
using prior knowledge; the second step is selecting candidates using a selection algorithm.
A method to rigorously define candidate sets is introduced. Suppose y(x1, x2, x3 ) is a model with three independent
variables and unknown model structure, the candidate set of y can be denoted by S(y). In this research, the model
structures are in the form of polynomial functions. Now denote the basis of a dth order polynomial function of
x = (x1, x2, x3 ) as P d (x) and then the candidate set consisting of arbitrary polynomial terms can be defined. For example,
if S(y) = {P2 (x1, x2 ), P2 (x1, x2 )x3 }, the candidate set of y contains regressors from P2 (x1, x2 ) = {x1, x2, x3, x12, x22, x1 x2 }
and from P2 (x1, x2 )x3 = {x1 x3, x2 x3, x3 x3, x12 x3, x22 x3, x1 x2 x3 }. Define the multiplication of two sets as a set containing
non-repetitive products of elements from the two sets
{a1, a2 ..., am }{b1, b2 ..., bn } = {a1 b1, ...a1 bn, a2 b1, ..., a2 bn, ..., am b1, ..., am bn }

(19)

Then S(y) can be expressed in a simplified form according to the law of association

S(y) = {P2 (x1, x2 ), P2 (x1, x2 )x3 } = {P2 (x1, x2 ){1, x3 }}

(20)

A general formulation of P d (x) is
pd (x1, x2, ..., xn ) = {

n
Ö

n
Õ

xiki |0 ≤

ki ≤ d, k i ∈ {0, 1, 2, ..., d}}

(21)

i=1

i=1

of which the total number of elements can be calculate by
(d + n)!
dˆ =
n!d!

(22)

After determining the candidate set, a so-called (forward-backward) stepwise regression algorithm is applied to
select regressors to build the model. The algorithm is summarized in the Appendix. Readers may refer [25] for more
details.

IV. Data acquisition and analysis
A. Experimental Setup
In order to identify force and moment models in high-speed flight regimes, free flights are performed in a large-scale
wind tunnel for data acquisition. The tested quadrotor is Parrot Bebop without bumper, as Fig. 2 shows. The native

10

autopilot of this off-the-shelf drone is replaced by Paparazzi [29], an open-source autopilot which runs at 512 Hz, and
which is capable of performing aggressive maneuvers. Incremental Nonlinear Dynamic Inversion (INDI) guidance
law and attitude controller have been programmed in Paparazzi [3, 28] to guarantee the position tracking performance
against strong wind, which is essential for flight tests in our research. The quadrotor is equipped with a closed-loop
Brushless DC motor controller; an MPU6050 Inertia Measurement Unit (IMU) including a 3-axis accelerometer and
gyroscope. The inertia of Bebop is measured using the approach introduced in Ref. [30] with a percent error less than
5%. These parameters of tested quadrotor are listed in the Table 1
Flight tests are performed in the Open Jet Facility (OJF), a large-scale wind tunnel, operated by Delft University of
Technology. The drone is controlled to maneuver in a confined area that is approximately 5.0 m long, 2.5 m wide and
2.5 m high. Wind speed is varied from 0 to 14 m/s with 2 m/s intervals to simulate flights at different airspeeds. An
external motion capture system is applied to measure velocities and positions of the drone for indoor navigation. As
Fig. 3 shows, five waypoints in the flight area are set to conduct flight maneuvers. To perform longitudinal maneuvers,
the quadrotor can be controlled to track point A and B alternately. Similarly, waypoints C and D are set for performing
lateral maneuver. To perform vertical maneuvers during forward flight, the drone is controlled to stay at point O and
climb or descend with 2 m/s. Longitudinal, lateral and vertical maneuvers are conducted at varying heading angles
denoted by ψ which are defined in Fig. 3; the heading angle is increased from 0 to 360◦ in steps of 45◦ . To identify the
yawing moment model considering aerodynamic effects, yaw maneuvers are carried out at point O by changing ψ in
steps of 45◦ both clockwise and counter-clockwise.

B. Data Preprocessing
Flight data for system identification are collected by on-board and external sensors. Specifically, rotor speeds
are observed by the motor controller, angular rates and specific forces are measured by gyroscope and accelerometer
respectively. These measurements are logged on-board at 512 Hz. The external motion capturing system (10 × OptiTrack
Prime 17 W cameras) measured the position of 6 markers fixed on the vehicle at 360 Hz, with a standard deviation less
than 0.2 mm. Henceforth the quadrotor position, attitude and ground speed are derived from these marker positions and
re-sampled to 512 Hz to align with on-board measurements.
Measurements from the two sources have been fused using an Extended Kalman Filter (EKF) with the aim of
calculating IMU bias [27]. The unbiased IMU measurements are further filtered by a 4th-order Butterworth lowpass
filter. Power spectral density (PSD) of accelerometer and gyroscope measurements from one flight are plotted in Fig. 4.
Table 1
m [kg]
0.389

Ix [kg · m2 ]
0.000906

Inertia and geometric parameters of Parrot Bebop

Iy [kg · m2 ]
0.001242

Iz [kg · m2 ]
0.002054

Ixz [kg · m2 ]
1.42E-05

11

I p [kg · m2 ]
3.39E-06

b [m]
0.0775

l [m]
0.0975

R [m]
0.064

PSD of a x (dB/Hz)

Fig. 3 Top view diagram of flight maneuvres performed in the wind tunnel. Forward and backward flights are
conducted by tracking point A and B by turns. Lateral flights are performed between C and D. Descend and
ascend flights, yaw maneuvres are made at point O. The heading angle ψ is defined as the angle between xB and
xG (clockwise positive).
20
0
-20
-40
-60

PSD of q(dB/Hz)

10 -2
20

10 0

cutoff frequency

10 2

5

0
-20
-40
-60
10 -2

10 0

16

10 2

frequency (Hz)

Fig. 4 Power spectrum density (PSD) of accelerometer measurement and gyroscope measurement (accx and
q) by Welch’s method in Matlab. Cut-off frequencies are chosen as 5 Hz and 16 Hz respectively. Circled parts
indicate the noise caused by an unbalanced rotor which is inevitable.
There is a resonance peak at around 120 Hz which is most likely caused by rotor imbalance (rotors rotate at around 7000
RPM = 117 Hz). Filter cut-off frequencies are chosen as 5 Hz and 16 Hz respectively, leading to a considerable noise
reduction as shown in Fig. 5.
Finally, force and moment measurements are derived from the processed IMU data. The specific force times the
mass of quadrotor equals the resultant non-gravitational force, namely F. The moment M can be obtained from Eq. (4)
where angular velocity Ω is obtained from the processed gyroscope measurement.

12

acc x (m/s 2 )

5

raw
filtered

0
-5
50

55

60

55

60

65

70

75

65

70

75

q (rad/s)

5

0

-5
50

time (s)

Fig. 5

Comparison between raw and filtered measurements from accelerometer and gyroscope.

C. Complex Aerodynamic Effect
As outlined in the introduction, complex aerodynamic effects such as interactions between quadrotor components,
have been clearly observed from the flight test data. These effects can significantly affect external forces and moments
and has to be taken into account when creating a high fidelity model.
0.03
α ∈ [-5,0]°
α ∈ [-20,-15] °
α ∈ [-45,-40] °

Ct

0.025

0.02

0.015
-100

-80

-60

-40

-20

0

20

40

60

80

100

β (deg)

Fig. 6 Ct vs. sideslip angle in different angle of attack.
Thrust coefficients Ct and corresponding sideslip angles β are shown in Fig. 6. These data are divided into three
groups according to angles of attack α. The intervals of α are presented in the figure as well, while the interval of
advance ratio is set as µ ∈ [0.1, 0.11] (V ≈ 5 m/s). Note that only data within these intervals are plotted. Trend lines are
also given for a better illustration of the Ct variation in different β. Apart from the vertical shift of Ct , the effect of β on
Ct also varies with α. When α is negative with large absolute value, the mean value of Ct seems uncorrelated to β. As α
is approaching zero, data with larger | β| have smaller Ct , which could be interpreted as the thrust degradation caused by
disturbance from the fuselage of Bebop quadrotor in high sideslip flights when the aft rotors are within the fuselage wake.

13

In contrast, when α is decreased and becomes sufficient small, the aft rotors are outside the fuselage wake, therefore the
sideslip angle does not influence the thrust coefficient. During the experiment, a clear shrill sound was produced while
the aft rotors were obstructed by the fuselage, which can be regarded as another evidence that supports this hypothesis.
q
Fig. 7 presents the horizontal force coefficient defined as Ch = Cx2 + Cy2 which indicates the total aerodynamic
resistance projected on the xB -yB plane. The angle of attack is selected around -20◦ which is typical during forward
flight. The distribution of data points with small sideslip angle (| β| ∈ [0, 10]◦ ) shows an almost linear relationship
between Ch and horizontal advance ratio µh . As sideslip angle grows, the drag coefficient also increases, and when
| β| ∈ [80, 90]◦ a quadratic tendency appears. This may be due to the fact that the fuselage of the Bebop has larger a
projection area on yB direction, as Fig. 2 shows.
Besides the sidesilp angle, the angle of attack also affects Ch . Since a large portion of the drag is produced by the
rotors, thrust degradation caused by aerodynamic interactions can lead to induced drag reduction and subsequently
decrease the total drag force. This drag reduction was not only observed in large sideslip angle but also straight forward
flights when fuselage wake does not influence rotors. As can be seen in Fig. 8 where the sideslip angle is small
(β ∈ [−10, 10]◦ ), however Ch is found reduced when α is close to or above zero. This might be explained by the fact
that the front rotors can obstruct the aft rotors and degraded their aerodynamic characteristics as well. Note that this
drag force reduction is not obvious in the low-speed region since the flight speed is not high enough for this interaction.
The drag force variance due to interactions should be considered as well in a high fidelity model.
0.015
|β| ∈ [80,90] °
|β| ∈ [0,10] °
|β| ∈ [40,50] °

Ch

0.01

0.005

0
0

0.05

0.1

0.15

0.2

0.25

0.3

0.35

µh

Fig. 7 Ch vs. µh in different sideslip angle intervals. Plot shows that horizontal force coefficient varies with
respect to sideslip angle.
Fig. 9 presents the pitching moment coefficient Cm versus the advance ratio µ at different sideslip angles. Significant
differences can be found in different β intervals. For these data, pitch control uq are chosen close to zero (uq ∈ [−0.05, 0])
which means that almost no pitching moment is present in hovering condition because the front and aft rotors are
nearly at the same speed. It is evident that the pitching moment increases significantly as the flight speed grows when

14

0.014
α = -20±0.1°
α = 0±0.1°

0.012

α = 10±0.1°

0.01

Ch

0.008

0.006

0.004

0.002

0
0

0.05

0.1

0.15

0.2

0.25

0.3

0.35

µh

Fig. 8 Ch vs. µh in different angles of attack when sideslip angle β ∈ [−10, 10]◦ . A larger angle of attack can
reduce the horizontal force coefficient.
| β| ∈ [−10, 10]◦ compared to other sideslip angles. This pitch-up moment might be caused by blade flexibility [11] or
interactions between rotors.
3

×10 -3
β ∈ [-10,10] °

2.5

β ∈ [50,60]

°

β ∈ [80,90] °

2
1.5

CM

1
0.5
0
-0.5
-1
-1.5
-2
0

0.05

0.1

0.15

0.2

0.25

0.3

0.35

0.4

µ

Fig. 9 Cm in different sideslip angles when uq ∈ [−0.05, 0]. Large nose up moment can be observed when
sideslip angle is small (β ∈ [−10, 10]◦ ).

The angle of attack is found to be positively related to the pitching moment during forward flight. Fig. 10 shows the
data and trend lines of Cm versus α with uq ∈ [−0.01, 0.01] and β ∈ [−10, 10]◦ . In these cases, the aft rotors and front
rotors have almost the same rotor speeds. In general, Cm is positively correlated with the angle of attack indicating the
longitudinal instability of a quadrotor. The slope is larger with a higher advance ratio, which is consistent with results
given by Fig. 9. More importantly, the Cm is almost always positive, indicating that a nose up aerodynamic moment
appears in quadrotor forward flight even with a large negative angle of attack. This coincides with the result from
Ref. [22] that the aft rotors need to rotate much faster than the front rotors in trim conditions. Similarly, for instance,
flying to the right (with β = 90◦ ) can produce a negative rolling moment which requires the left rotors to rotate faster in
15

order to keep balance. This phenomenon is strongly present in the data; simple hovering models that neglect this effect
will produce highly inaccurate moment predictions in fast flight regions.
5

×10 -3

4.5

µ∈[0.05,0.10]
µ∈[0.10,0.15]

4
3.5

CM

3
2.5
2
1.5
1
0.5
0
-50

-40

-30

-20

-10

0

10

α [deg]

Fig. 10 Cm vs. α in different advance ratio intervals with uq ∈ [−0.01, 0.01] and β ∈ [−10, 10]◦ .
The yawing moment is also found to be influenced by the sideslip angle. Notate the difference between the measured
yawing moment and that calculated by the hovering model by ∆Mz , which is regarded as the additional yawing moment
due to the aerodynamic effects.
∆Mz = Mz − [τ0 (−Ω21 + Ω22 − Ω23 + Ω23 ) + λr r]

(23)

As is shown in Fig. 11, ∆Mz is negatively related to the sideslip angle in general. When | β| < 40◦ , ∆Mz is in the
vicinity of zero. However its dispersion suddenly increases when | β| > 40◦ . At the same time, one aft rotor starts to be
obstructed by the fuselage. The airframe-rotor aerodynamic interaction might occur in this situation as the cause of the
sudden increased yawing moment.

V. Quadrotor Model Structure Candidates
This section introduces the determination of candidate structure sets for the quadrotor dimensionless aerodynamic
force and moment models, based on prior physical knowledge and observations. Then, the stepwise regression algorithm
can be carried out to obtain the final model structure, which will be presented in a subsequent section.

A. Force Model Candidates
Fx and Fy denote forces perpendicular to the thrust direction brought by aerodynamic resistance, of which the
lift-induced drag and the blade flapping effect are the two major causes [6, 12, 19]. A widely accepted drag model of a

16

0.05
0.04
0.03

∆M z (Nm)

0.02
0.01
0
-0.01
-0.02
-0.03
-0.04
-0.05
-100

-80

-60

-40

-20

0

20

40

60

80

100

β (deg)

Fig. 11 Additional aerodynamic yawing moment ∆Mz with respect to the hovering model. ∆Mz is negative
related to the sideslip angle indicating that an aerodynamic moment related to β exists.
single rotor is [6, 12]
Fx,i ∝ ui Ωi

(24)

where Fx,i and ui stand for the in-plane force and local velocity of the i th rotor on the xb direction respectively. In
addition, as section C shows, airframe affects the drag force as well, especially at large sideslip angles. Therefore, the
square of the velocity has been added into the drag model [16]. The drag force on xB direction can thus be expressed as

Fx = κd,1

4
Õ

Ωi ui + κd,2 u2

(25)

i=1

where κd,1 and κd,2 are constants. Recall Eq. (8911), the normalized form of Eq. (25) can be obtained
Cx = Cx,1 µx + Cx,2 µ2x

note that the rotor speed term

(26)

Í4

i=1 Ωi in Eq. (25) disappears after normalization. This process needs to replace the

arithmetic mean of rotor speeds by their geometrical mean, which does not lose accuracy in most flight conditions
(0.8% relative error on average).
The above model can produce accurate predictions in most low-speed cases. However, it loses accuracy when
interaction effects appear. For comparison with the gray-box model established in this research, the drag model in
Eq. (26) is named the reduced model.
An additional term Cx,2,(µ x , |µy |,µz ) , of which the exact structure is to be determined, is added to the reduced model.
The variables in the subscript parentheses indicate the independent variables of Cx,2 . A preliminary model structure of

17

Cx can be
Cx = Cx,1 µx + Cx,2,(µ x , |µy |,µz )

(27)

Note that the Cx,2 µ2x term is moved into the second term since it has a negligible effect in the flight regime as the data
shows.
Recall the observation in the Section. C; β, α and µh greatly influence the drag coefficient. Singularities in α and
β, however, could occur in hovering conditions, therefore three components of the advance ratio are chosen as the
independent variables. Absolute values of µy are used due to the symmetry of the quadrotor.
Similarly, for Cy we have
Cy = Cy,1 µy + Cy,2,(|µ x |,µy ,µz )

(28)

The stepwise regression algorithm can be applied to determine the exact structure of the unknown parts of the above
models. The candidate sets of Cx and Cy are chosen as
S(Cx ) = {P3 (µx, | µy |, µz )}

(29)

S(Cy ) = {P3 (| µx |, µy, µz )}

(30)

The terms µx and µy are always the first candidates to be tested by the model structure selection algorithm when
assembling respectively the models for Cx and Cy .
Fz is derived by taking account of the thrust variation. In general, the thrust of the ith rotor with constant pitching
angle can be expressed as [31]
Ti =

ρaBcωi2 R3 θ r (ui2 + vi2 )θ r −wi + νin,i
( +
)
+
2
3
2ωi R
2ωi2 R2

(31)

where a is the lift curve slope of the blade profile, B represents the number of blades, c is the blade chord length and θ r
stands for the rotor pitch angle. These parameters are related to rotor design and for quadrotors; they normally are all
constants. νin,i indicates the induced velocity of the ith rotor.
A common way to model Fz is projecting the thrust of four rotors on the body frame and assuming that local
velocities and induced velocities of the rotors are identical, yielding

Fz = −

4
Õ
i=1

Ti = κt,1

4
Õ

Ω2i + κt,2 (u2 + v 2 ) + κt,3 (−w + νin )

i=1

4
Õ

Ωi

(32)

i=1

where κt,1 , κt,2 and κt,3 are constants. Eq. (32) is the model structure adopted in several researches [13, 31]; it is
indicated as the reduced model to compare against the new gray-box model. A dimensionless form of Eq. (32) can be

18

Table 2

Procedure of identifying the reduced model and the gray-box model
Reduced model

step 1
step 2
step 3

Gray-box model
Data acquisition
Determined structure Define structure candidates S(y)
Parameter estimation
Stepwise regression
Final model

calculated by substituting Eq. (8,9,11) into Eq. (32), yielding
Cz = Cz,0 + Cz,1 (µ2x + µ2y ) + Cz,2 (−µz + ν̄in )

(33)

where Cz,0 , Cz,1 and Cz,2 are constants. ν̄in indicates the dimensionless induced velocity normalized by Ω̄R. The
induced velocity ν̄in can be calculated by [4]
Ct,h
ν̄in = q
2 µ2x + µ2y + (−µz + ν̄in )2

(34)

where Ct,h is the thrust coefficient in the hover case which can be estimated accurately by conducting hovering flight
experiments.
As mentioned earlier, interaction effects could degrade rotor thrust. This effect is not considered in the reduced
model (32), not to mention other unknown complex aerodynamic effects and drag force on the z B direction. The
deviation of Cz from these effects is denoted by Cz,3 . Flight speed, the difference between aft and front rotors, vehicle
angular velocities could be the individual variables of this unknown part. Thus a gray-box model of Cz can be formalized
as
Cz = Cz,0 + Cz,1 (µ2x + µ2y ) + Cz,2 (−µz + ν̄in ) + Cz,3,(|µ x |, |µy |,µz , |u p |, |uq |, |ur |, |p |, |q |, |r |)

(35)

Henceforth the stepwise regression algorithm is applied to determine the structure of Cz , of which the candidate
structure set is chosen as
S(Cz ) = {P4 (| µx |, | µy |, µz ){1, | p̄|, | q̄|, |r̄ |, |u p |, |uq |, |ur |}}

(36)

Note that (µ2x + µ2y ) and (−µz + ν̄in ) are regarded as fixed regressors that have been added in the model before the
selection.
Both the reduced model and the gray-box model can be established using the system identification method. Table 2
briefly compares the procedures of establishing these models.

19

B. Moment Model Candidates
Compared to the hovering case, additional aerodynamic moments can be produced during high-speed flight. First,
the pitching moment could be a result of rotor resilience and the blade flapping effect [11]. Secondly, the vertical
distance between rotor planes and the center of gravity also brings moments due to rotor drag [21]. Thirdly, angular
rate (dynamic) damping term also contributes to the total aerodynamic moment [21]. Fourthly, as was observed from
the flight test data, aerodynamic interactions may degrade the thrust of the aft rotors and lead to additional pitch up
moments. All the above factors are related to advance ratio and angular rates.
Taking account of these possible effects, a lumped model of the pitching moment coefficient Cm could be expressed
as
Cm = Cm,(µ x ,µy ,µz , q̄,uq )

(37)

with advance ratio, dimensionless pitch rate and pitch input as independent variables.
Above pitch model neglects the influence of lateral variables u p , ur , p and r, which are of less effect on the pitching
moment based on the flight data. However, µy is included in the model to handle the sideslip effect. To further simplify
the candidate set, uq and q̄ are assumed linearly related to the pitching moment. Thus the lumped model (37) can be
expressed as
Cm = Cm,0,(µ x , |µy |,µz ) + Cm,uq ,(µ x , |µy |,µz ) uq + Cm,q,(µ x , |µy |,µz ) q̄

(38)

Based on the structure in Eq. (38), the candidate set of Cm is chosen as
S(Cm ) = {P5 (µx, µz )P2 (| µy |){1, q̄, uq }}

(39)

The preliminary structure of the rolling moment model can be similarly determined as
Cl = Cl,0,( |µ x |,µy ,µz ) + Cl,u p ,(|µ x |,µy ,µz ) u p + Cl, p,(|µ x |,µy ,µz ) p̄

(40)

of which the regressors are selected from the candidate set
S(Cl ) = {P5 (µy, µz )P2 (| µx |){1, p̄, u p }}

(41)

The yawing moment model is found to be much more complicated and ur may not be linear to the model. Thus a
preliminary structure is determined as
Cn = Cn,(µ x ,µy ,µz , r̄,ur )

20

(42)

of which the candidate set is chosen as
S(Cn ) = {P5 (µx, µy, µz )P3 (r̄)P3 (ur )}

(43)

VI. Results
A. Model Estimation Results
After defining candidate structure sets, all force and moment models are determined by the stepwise regression
algorithm. This section provides the estimation result of Cz since thrust is the biggest concern in most modeling tasks.
Due to limited space, other models are provided in the Appendix.
The reduced model is found to be accurate in low-speed regions since the interaction effects are weak. Thus Cz is
estimated with structure (33) when µ < 0.05 (approximately V < 2 m/s). The result at this low-speed regime is listed in
Table 3.
For the flight regime where µ > 0.05, the complex aerodynamic effects become apparent and the gray-box model of
Cz is established. The model structures and parameters are listed in Table 4. The state space is equally divided into
three partitions according to the sideslip angle. On each partition, a gray-box model of Cz has been identified. The first
column of the table lists the model structure which is ranked by the order of selection. The second column gives the
values of corresponding parameters. The third column provides the decreasing normalized-root-mean-square (NRMS)
of the model residual after the corresponding regressor is added into the model. In general, regressors at the top are the
most significant, with significance becoming less moving towards the bottom of the list.
Fig. 12, 13 present the residuals of gray-box models on the entire estimation data sets. 1 − σ intervals are given as
well. In general, the residuals are confined to the interval. It also indicates that the gray-box model provides unbiased
estimations because all residuals have a mean value closed to zero. Above properties demonstrate the validity of the
model structure selected by the stepwise regression algorithm.
Table 3

Estimation results of Cz model, µ ≤ 0.05
θ̂
5.020E-02
1.112E-01
-9.420E-02
0.953
0.021

reg.
1
2
µx + µ2y
(ν̄in − µz )2
R2
NRMS

21

ǫ (Cx )

×10 -3
1
0
-1
0

500

1000

1500

2000

500

1000

1500

2000

1000

1500

2000

ǫ (Cy )

×10 -3
1
0
-1

ǫ (Cz )

0
4
2
0
-2
-4

×10 -3

0

500

time [s]

Fig. 12

Residuals of force models compared with 1-σ interval.

ǫ (Cl )

×10 -3
1
0
-1
0

500

1000

1500

2000

500

1000

1500

2000

1000

1500

2000

ǫ (Cm )

×10 -3
1
0
-1
0

ǫ (Cn )

1

×10 -3

0
-1
0

500

time [s]

Fig. 13

Residuals of moment models compared with 1-σ interval.

22

Table 4

reg.
1
µ2x + µ2y
(ν̄in − µz )2
µz
|uq | µz
|u p | µz
|u p | µ2z
|u| µ3z
|uq |
R2

| β| ∈ [0, 30]
θ̂
NRMS (%)
1.38E-01
8.636
-1.55E-01
8.487
-4.00E-01
2.315
-1.02E-01
2.070
-2.28E-02
1.995
7.83E-02
1.952
5.88E-01
1.940
3.48E+00
1.928
-3.11E-04
1.923
0.9503

Estimations result of Cz model, µ > 0.05
| β| ∈ [30, 60]
reg.
θ̂
NRMS (%)
1
2.20E-01
8.046
µ2x + µ2y
-3.62E-01
7.487
2
(ν̄in − µz )
-6.99E-01
2.142
µz
-2.91E-01
1.785
|u p || µy |
-1.84E-02
1.723
|ur || µy | 2 µ2z 3.22E+00
1.696
|uq || µx |
1.19E-02
1.672
3
|r̄ || µy | µz
1.34E+03
1.655
|u p || µy | 2 µ2z 3.81E+00
1.650
R2
0.9580

Table 5

Fz
Fx
Fy
Mx
My
Mz

| β| ∈ [60, 90]
reg.
θ̂
NRMS (%)
1
1.48E-01
8.759
µ2x + µ2y
-2.05E-01
8.533
(ν̄in − µz )2
-4.48E-01
2.882
µz
-1.93E-01
2.575
|u p | µz
4.52E-02
2.418
µ3z
1.08E+01
2.342
µ4z
3.70E+01
2.240
2
µz
5.82E-01
2.165
|u p || µy |
7.06E-03
2.148
3
|u p || µy | | µz | -1.51E+00
2.139
R2
0.9403

Summary of validation results

gray-box model
Output corr.
R2
NRMS (%)
0.9353
0.8636
2.10
0.9945
0.9889
1.54
0.9987
0.9975
1.05
0.7687
0.4847
2.06
0.8567
0.6883
1.23
0.8071
0.4873
5.19

reduced or. hovering∗ model
Output corr.
R2
NRMS (%)
0.8831
0.7650
3.03
0.9934
0.9861
1.70
0.9981
0.9961
1.32
∗
∗
0.1944
-0.0201
12.63∗
0.4141∗
-0.0524∗
7.53∗
∗
∗
0.4152
0.1417
13.81∗

B. Validation Results
Gray-box models are validated using validation data which are separate from the estimation data but are collected
from the same flights. For a better evaluation of these models, the validation outputs are chosen as forces and moments
instead of their coefficients.
The force models are compared with the reduced models (25, 32) which have taken into account primary aerodynamic
effects. The moment models, however, are compared with the hovering model because no mature reduced model for
moment prediction can be found. Metrics of these models are given in Table 5 based on validation results.
Both gray-box and reduced model provide accurate Fx and Fy predictions. From the Fz metrics, it can be concluded
that the gray-box model provides better thrust estimations. The RMS of the gray-box force model residuals are reduced
by 20-30% on the whole. As for moment predictions, the hovering models (marked by an asterisk) are almost invalid
while the gray-box models can provide adequate results. Specifically, the RMS of residuals are reduced by over 80%.
To make detailed comparison between above models, several figures are given below.
Fig. 14 gives the validation result of Fz where thrust T = −Fz is plotted for readability. Climbing and descending

23

Fig. 14 Validation results of the thrust model. The gray-box model (red solid) is compared with the reduced
model(green dash-dot) and the hovering model (black dash). The left two figures illustrate flights when wind
speeds are 5 m/s and 10 m/s and the sideslip angle is zero. The right two figures present flights when the sideslip
angle is 90◦ . The gray-box model outperforms the other models in general.
flights are performed at 5 m/s and 10 m/s airspeeds with β = 0 and β = −90◦ respectively. It’s clear that both the
gray-box and the reduced model (32) outperform the hovering model, especially in the high-speed and large sideslip
flight regime. It is evident that accuracy of the reduced model degrades beyond 5 m/s which could be caused by
disturbances from the interactions of the Bebop fuselage with the airflow.

Fig. 15 Validation results of the Fx model. The gray-box model (red solid line) is compared with the reduced
model (green dash-dot). Forward and backward flights are performed for validation. The left two figures show
flights with zero heading angle, i.e towards the wind tunnel outlet, while the heading angle is 45◦ in the right
figures.

24

α (deg)

50

0

-50
0

1

2

3

4

5

6

7

8

9

10

5

6

7

8

9

10

V = 10m/s ψ = 0 °

model residual

0.4
gray-box model
reduced model

0.3
0.2
0.1
0
0

1

2

3

4

time (s)

Fig. 16 Time series of angle of attack at Vwind = 10 m/s and ψ = 0. The angle of attack is positive at t = 5.0 s
and t = 8.3 s when errors in the reduced model for Fx appear.
The drag model has been validated by forward and backward flights with different heading angles ψ. Fig. 15 shows
the validation results of Fx . The reduced model (25) neglecting interaction effects are compared. Although both models
perform well in general, the gray-box model is more accurate in certain parts. For example, at t = 5.0 s and t = 8.3 s in
the Fig. 15b, the gray-box provides accurate prediction while the reduced model produces relatively large errors. The
angle of attack and the model residuals of this subplot are given in Fig. 16. It can be seen that the gray-box model
outperforms the reduced model when the angle of attack is positive, which means that interactions between the front and
aft rotors appear and the reduced model becomes less accurate.
Fig. 17 provides validation results of the Fy model. In flights with 90◦ heading angles, the quadrotor flew towards
the left as what Fig. 3 illustrates. Again, as what is shown in Fig. 18, the gray-box model outperforms the reduced
model at points when α is above zero. At t = 12 s when angle of attack is positive, a large error appears in predictions
made by the reduced model, which does not take into account aerodynamic interaction effects.
Fig. 19 presents the validation result of pitching moment model. The gray-box model outperforms the hovering
model as expected. During flights with ψ = 0, the prediction of the hovering model is almost always smaller than the
measurements since the aft rotors need to rotate much faster than front rotors in the trim condition, which is in line with
the observation given in Ref. [22]. This phenomena indicates that during forward flight, a significant pitch up moment
appears which is not considered in the hovering model. As for the flights with ψ = 45◦ , although a large aerodynamic
coupling exists due to large sideslip angles, the gray-box model can still provide accurate predictions. In addition,
validation results of the Mx model are given in Fig. 20 showing the great advantage of the gray-box model.
The model of the yawing moment Mz has been validated by yaw maneuvers and forward-backward maneuvers in the

25

Fig. 17 Validation results of the Fy model. The gray-box model (red solid line) is compared with the reduced
model (green dash-dot). Forward and backward flights relative to the wind flow are performed. The left two
figures present flights with 90◦ heading angle, i.e leftward flight against wind flow, while the heading angle is 45◦
in the right figures.
wind tunnel. The forward-backward maneuvers are carried out with ψ = 45◦ . As can be seen in Fig. 21, predictions
from the gray-box model are more accurate than the hovering model. The residual of the hovering model increases as
the flight speed grows and a constant bias appears in the Fig. 21d. In this case, the quadrotor flies with ψ = 45◦ , in other
words, negative sideslip angle and additional positive yawing moment appear due to aerodynamic effects.
Finally, the gray-box models have been validated near the hovering condition and compared with the hovering
models. As Fig. 22 shows, both types of models are accurate. Since aerodynamic resistance is small compared to the
presented variables in the hovering condition, models of Fx and Fy are omitted in the plot.
The piecewise polynomial model in this research is discontinuous on the boundary of each section that might be
unfavorable for some applications, though the discontinuity can be effectively weakened by increasing the number of
model segments. More advanced base functions such as multivariate splines may replace polynomials to guarantee the
smoothness of the global model.

VII. Conclusions
Gray-box models of a specific type of quadrotor considering aerodynamic interaction effects have been identified
from flight data in a larger flight envelope with respect to the hovering condition. The identification process of this
gray-box model consists of the information of phenomenological observation and prior-knowledge about rotorcraft
aerodynamics. Therefore, this type of model possesses higher reliability in both high speed and low-speed flight
regimes. Although the model structure and parameters are specific to the Bebop platform, the methodology including

26

20

α (deg)

0
-20
-40
-60
0

5

10

15

10

15

model residual

0.3
gray-box model
reduced model

0.2

V = 10m/s ψ = 90 °
0.1

0
0

5

time (s)

Fig. 18 Time series of angle of attack in Vwind = 10 m/s and ψ = 90◦ . The angle of attack is positive at t = 12.0 s
when errors of Fy reduced model appear.

Fig. 19 Validation result of pitching moment My model. The gray-box model (red solid) is compared with
hovering model (black dash-dot). Forward and backward flights relative to the wind flow are used for validation.
The left two figures show flights with zero heading angle, i.e towards the wind tunnel outlet, while heading angle
is 45◦ in the right figures.
nondimensionalization, structure candidates and stepwise regression algorithm, can be generalized to other multi-rotor
platforms.
The high-speed flight data have been collected in the wind tunnel. These flight data illustrate significant interaction
effects which were rarely considered in the previous literature. The thrust reductions of the aft rotors that obstructed by
the front rotors or airframe lead to variations of thrust, moment and even drag force acting on the quadrotor, which may

27

Fig. 20 Validation result of rolling moment Mx model. The gray-box model (red solid) is compared with
hovering model (black dash-dot). Forward and backward flights relative to the wind low are performed. The
left two figures present flights with 90◦ heading angle, i.e leftward flight against wind flow, while the heading
angle is 45◦ in the right figures.

Fig. 21 Validation results of the Mz model. The gray-box model (red solid) is compared with hovering model
(black dash-dot). Yaw maneuvers are performed in the left two figures, with different flight speeds. Right two
figures present data from forward and backward flights along the wind flow direction with 45◦ heading angle.
inspire the drone manufacturer to revise their design. For instance, increase the power of aft actuators.
Since the data are obtained from flight tests instead of conventional static wind tunnel tests, and the forces and
moments are measured indirectly from on-board and external navigation sensors, this process can also be applied in the
open area instead of the wind tunnel. The motion capture system can be replaced by other navigation sensors, such
as RTK-GPS, which can produce accurate velocity measurements. Thus the method introduced in this article can be

28

0.2

M y (Nm)

T (N)

5
4
3

0.1
0
-0.1
-0.2

0

1

2

a.)

3

4

time (s)

0

M z (Nm)

0
-0.1
-0.2

1
b.)

measurement
gray-box model
hovering model 0.05

0.1

M x (Nm)

5

2

3

4

time (s)

0

-0.05
0
c.)

1

2

3

4

0

2
d.)

time (s)

4

6

8

10

time (s)

Fig. 22 Validation results during hovering condition. Gray-box model (red solid) is compared with hovering
model (green dash-dot). Both model possess enough accuracy in hovering condition.
repeated without wind tunnel equipment to establish accurate models in the interested flight regime.
However, on the other hand, a flight test is only able to explore a limited regime of the flight envelope, which is
unfavorable for global model identification. The interaction effects can be only partially revealed by free flight data.
Therefore, static wind tunnel tests with force balance are also suggested to carry out for global model identification as
well as analyzing interaction effects in detail.

Appendix
A. Stepwise Regression Algorithm
The stepwise regression algorithm is summarized in the Algorithm 1.

B. Estimated Model of Cx , Cy , Cm , Cl and Cn
The estimated aerodynamic coefficients are given in this appendix. Note that the model structure and corresponding
parameters are only applicable to Parrot Bebop without bumpers.

29

Algorithm 1 Forward-backward Stepwise Regression Algorithm
Set initial regressor matrix A0 = [1, 1, ..., 1]T ∈ R N
Set candidate set S(y) = {ξ 0, ξ 1, ..., ξ q } containing q + 1 candidates
k max = 30; PSEtol = 10−6 ; Fout = 4; k = 0
 0 = [I − A0 (AT0 A0 )−1 AT0 ]z
while k ≤ k max do
%Forward selection%
k = k +1
for i = 0, 1, ..., q do
λ i = ξ i − Ak−1 (ATk−1 Ak−1 )−1 ATk−1 z
end for
j = argmaxi corr(λ i,  k−1 )?
. %corr(x, y) stands for the correlation of x and y%
Ak = [Ak−1, ξ j ]
θ̂ = (ATk Ak )−1 Ak z
 k = z − Ak θ̂ k
%Backward elimination%
Assume there has been p regressors added into Ak
for i = 1, 2, ..., p do
Define Ak,i as the reduced regressor matrix of Ak of which the ith regressor is eliminated
θ̂ k,i = (ATk,i Ak,i )−1 ATk,i z
SSR (θ̂ k ) = θ̂ k ATk z − N z̄2
. % z̄ stands for the mean of z%
SSR (θ̂ k,i ) = θ̂ k,i ATk,i z − N z̄2
s2 =  Tk  k /(N − p − 1)
The F0-ratio of the ith regressor can be calculated by
F0,i = [SSR (θ̂ k ) − SSR (θ̂ k,i )]/s2
end for
l = argmini F0,i
if F0,l < Fout then Ak = Ak,l
θ̂ = (ATk Ak )−1 Ak z
 k = z − Ak θ̂ k
end if
%Stopping criteria%
ÍN
[z(i) − z̄]
PSE = N1  Tk  k + Np2 i=1
if PSE > PSElast or PSE ≤ PSEtol or l = j then
break
end if
PSElast = PSE
end while
Ak is the final regressors matrix (model structure), θ̂ k is the estimated parameters and  k is the model residual.

30

Table 6

reg.
1
µx
µ x µz
µx µ2z
µx µ3z
µ2x µ2z
µ3x
µ2z
R2

| β| ∈ [0, 30]◦
θ̂
NRMS (%)
4.182E-04
18.374
-3.482E-02
1.327
7.717E-02
1.232
1.057E+00
1.157
3.837E+00
1.119
7.365E-01
1.101
-2.883E-02
1.082
-2.073E-02
1.075
0.9966

reg.
1
µx
| µy | 3
µx µ2z
µ x µz
µx µ3z
µ3x
R2

Table 7

reg.
1
µy
µ y µz
µ2x µ2z
| µx | µ2z
| µx |
R2

| β| ∈ [0, 30]◦
θ̂
NRMS (%)
-1.79E-04
6.595
-3.36E-02
2.771
1.16E-01
2.718
9.29E-01
2.693
-2.10E-01
2.677
5.45E-04
2.664
0.8368

Estimation results of Cx model
| β| ∈ [30, 60]◦
θ̂
NRMS (%)
4.390E-04
22.466
-3.684E-02
2.271
7.354E-02
2.213
3.673E+00
2.190
2.216E-01
2.111
1.497E+01
1.929
6.234E-02
1.918
0.9927

reg.
1
µx
| µy | 3
µz
µ2z
µx µ2z
R2

| β| ∈ [60, 90]◦
θ̂
NRMS (%)
5.007E-04
9.566
-3.873E-02
3.067
2.412E-02
2.983
3.956E-03
2.960
3.055E-02
2.915
2.495E-01
2.904
0.9079

Estimation result of Cy model

reg.
1
µy
| µ x | µ y µz
µy µ3z
µ y µz
µy µ2z
µ3y
| µx |
R2

| β| ∈ [30, 60]◦
θ̂
NRMS (%)
-1.98E-04
16.820
-3.54E-02
1.675
2.56E-01
1.201
4.64E+00
1.163
1.61E-01
1.141
1.57E+00
1.113
-1.05E-01
1.089
1.14E-03
1.083
0.9959

31

reg.
1
µy
µ3y µz
µy µ3z
µ y µz
µy µ2z
µ3y
µ2y µ2z
R2

| β| ∈ [60, 90]◦
θ̂
NRMS (%)
-1.23E-04
16.563
-3.88E-02
1.283
4.41E-01
0.979
1.41E+00
0.945
8.39E-02
0.877
7.41E-01
0.857
-7.01E-02
0.830
1.12E-01
0.828
0.9975

Table 8

reg.
1
uq
u q µz
µx
µ2x
µz
q̄
µ3x
µ4x
uq µ2x µz
uq µ3z
uq µ4z
µ2x µz
uq µx µ3z
µ2x | µy | µ2z
µx µ2y µ2z
R2

| β| ∈ [0, 15]◦
θ̂
NRMS (%)
1.16E-04
6.247
4.44E-03
4.831
-5.10E-02
4.589
5.04E-02
4.414
1.91E-02
4.019
3.57E-02
3.375
-1.35E-01
3.205
-1.27E+00
3.072
2.87E+00
2.715
8.28E-01
2.500
9.31E+00
2.410
2.25E+01
2.301
4.11E-01
2.234
-2.31E+01
2.178
1.90E+02
2.120
-7.62E+02
2.088
0.8872

Estimation result of Cm model, | β| ∈ [0, 45]◦

reg.
1
uq
uq | µy |
µx
µ3x µz
µ3x µ2y µz
µ x | µy |
u q µz
q̄
uq µ2z
R2

| β| ∈ [15, 30]◦
θ̂
NRMS (%)
5.36E-05
9.779
4.70E-03
6.682
-5.31E-02
4.843
4.32E-02
3.473
5.60E+00
3.093
-5.82E+02
2.857
-4.29E-01
2.650
-6.93E-02
2.512
-6.94E-02
2.147
-3.25E-01
2.088
0.9521

32

reg.
1
uq
uq | µy |
uq µ2y
µx
u q µz
µx µ2z
uq µ2y µz
µx µ3z
q̄
3
uq µx µ2y µz
uq µ3z
uq µ2x
uq µ2x µ2y
µ2y µ5z
R2

| β| ∈ [30, 45]◦
θ̂
NRMS (%)
1.73E-05
5.456
5.01E-03
4.490
-9.56E-02
4.150
8.09E-01
3.168
2.84E-02
2.988
-6.23E-02
2.714
-5.20E+00
2.360
2.90E+00
2.154
-2.82E+01
1.947
-8.51E-02
1.768
3.53E+02
1.718
3.43E+00
1.619
1.46E-01
1.513
-8.85E+00
1.431
-8.23E+02
1.407
0.9393

Table 9

Estimation result of Cm model, | β| ∈ [45, 90]◦

| β| ∈ [45, 60]◦
reg.
θ̂
NRMS (%)
1
1.97E-04
3.290
uq
4.17E-03
2.837
µz
3.00E-03
2.306
µx
3.74E-02
2.128
7.33E+00
1.995
µx | µy | µ2z
uq µ2x | µy |
2.42E+00
1.900
u q µz
-6.30E-02
1.845
µx q̄
-8.64E-01
1.815
uq | µy | µ2z
-3.67E+00
1.788
µx µ2z
-5.88E+00
1.706
2
u q µ x µz
4.35E+00
1.653
-5.76E+01
1.631
uq µ3x µ2y
µ x | µy |
-1.03E-01
1.608
µx µ3z
-2.71E+01
1.550
uq | µy |
-2.04E-02
1.517
3
2
u q µ x µ y µz
-5.26E+02
1.476
uq µ2x | µy | µ2z 8.14E+01
1.438
2
5
-1.51E+03
1.425
u q µ y µz
R2
0.8107

| β| ∈ [60, 75]◦
reg.
θ̂
NRMS (%)
1
-3.82E-05
4.779
uq
4.73E-03
3.767
µx
5.52E-02
3.605
2
µ x µz
-7.24E+00
3.400
u q µz
-7.10E-02
3.244
µ x | µy |
-3.57E-01
3.100
uq | µy |
-2.66E-02
2.996
µx | µy | µ2z
5.24E+01
2.800
µx µ2y µ2z
-1.02E+02
2.641
3
2
µx µy q̄
-9.88E+03
2.544
2
u q µz
-3.92E-01
2.467
2.394
µ3x | µy | µz q̄ -1.77E+04
q̄
-6.07E-02
2.360
uq µ2y
8.98E-02
2.334
u q | µ y | µz
3.12E-01
2.315
uq µ x
-5.50E-03
2.257
µx µ2y
5.39E-01
2.236
µ2y µz
8.33E-02
2.185
uq | µy | µ2z
1.41E+00
2.163
µx µ3z
-5.46E+00
2.156
R2
0.7782

33

reg.
1
uq
µ2y µz
u q µz
uq µ2z
µ2y µ3z
uq | µy | µ4z
µz
| µy | µ5z
uq µ3z
uq µ4z
uq | µy |
uq | µy | µ2z
u q | µ y | µz
µ2y µ2z
| µy | µ4z
µx
R2

| β| ∈ [75, 90]◦
θ̂
NRMS (%)
-1.75E-04
3.590
3.67E-03
2.635
1.57E-01
2.615
-9.03E-02
2.598
-9.29E-01
2.562
-2.56E+01
2.512
-2.17E+02
2.491
-2.44E-03
2.470
2.41E+02
2.448
1.49E+01
2.435
1.14E+02
2.402
-1.06E-02
2.370
6.79E+00
2.351
2.45E-01
2.340
-2.11E+00
2.307
2.69E+01
2.298
1.58E-03
2.292
0.5958

Table 10

reg.
1
up
µy
| µx | p̄
µ2x µy
u p | µx |
u p µz
u p µ2x
u p µ2x µ2z
u p µy
µ2x µ2y
u p µ2y
u p µ2x µ3y
µ2x µ4y
µ2y p̄
R2

| β| ∈ [0, 15]◦
θ̂
NRMS (%)
2.30E-04
2.940
5.40E-03
1.885
-2.84E-02
1.825
-3.32E-01
1.789
5.08E-01
1.767
-4.44E-02
1.726
-1.26E-02
1.685
1.47E-01
1.659
-2.10E+00
1.629
-4.37E-02
1.607
2.36E+01
1.584
3.05E+00
1.550
1.75E+03
1.537
-8.40E+03
1.519
-9.54E+01
1.510
0.7331

Table 11

reg.
1
up
µy
| µx | µ3y µz
| µ x | µy
µ2x µy
u p µz
| µx | µ2y
µy µ3z
| µx | µy µ2z
p̄
µ3y
u p µ2z
u p | µx |
u p µ2y
µ2x µ2y
R2

| β| ∈ [45, 60]◦
θ̂
NRMS (%)
3.16E-04
3.626
4.32E-03
3.551
-3.45E-02
2.982
-2.93E+01
2.670
4.44E-01
2.592
-1.54E+00
2.512
1.03E-02
2.475
-4.09E-02
2.438
-7.70E+00
2.414
-1.02E+01
2.382
-5.76E-02
2.352
-4.58E-01
2.325
9.37E-02
2.305
-1.96E-02
2.288
6.65E-02
2.269
7.15E+00
2.262
0.6031

Estimation result of Cl model, | β| ∈ [0, 45]◦

reg.
1
up
µy
2
µx µ3y µz
µ2y
| µx | µ4y
p̄
µ2z p̄
R2

| β| ∈ [15, 30]◦
θ̂
NRMS (%)
1.28E-04
6.321
5.08E-03
3.693
-1.93E-02
2.695
-9.23E+02
2.186
2.56E-01
2.042
-1.80E+02
1.795
-3.72E-02
1.662
-8.79E+00
1.637
0.9282

reg.
1
up
µy
2
µ x µy
| µ x | µy
µ3y µz
µ2x µ3y
µz p̄
| µx | µy µ2z
u p µy
µ2x µ5z p̄
p̄
u p µ2z
R2

| β| ∈ [30, 45]◦
θ̂
NRMS (%)
2.79E-04
6.603
3.86E-03
6.251
-4.12E-02
5.089
-2.51E+00
4.567
5.41E-01
4.407
2.03E+01
4.289
4.74E+01
4.187
1.33E+00
4.048
1.54E+01
3.844
4.73E-03
3.768
2.70E+05
3.741
-4.30E-02
3.720
2.64E-02
3.704
0.6859

Estimation result of Cl model, | β| ∈ [45, 90]◦

reg.
1
up
µy
µ3y
µy µ2z
u p µ2y µ2z
u p µ2y µz
µ2y p̄
u p µ5z
µ3y µz
u p µ4z
µy µ3z
u p µ2z
u p µ3z
R2

| β| ∈ [60, 75]◦
θ̂
NRMS (%)
2.69E-04
6.058
4.37E-03
5.671
-2.36E-02
4.600
2.12E-01
3.826
2.37E+00
3.646
-3.61E+00
3.473
-6.41E-01
3.369
-3.25E+00
3.291
-1.66E+02
3.154
1.31E+00
3.124
-2.39E+01
3.050
9.49E+00
2.965
-2.79E-01
2.922
-1.14E+00
2.907
0.7632

34

reg.
1
up
µy
µ3y
µy µ2z
µ2y µz p̄
p̄
u p µ4z
u p µ3z
µz
u p µ2y
u p µ5z
µy µ3z
u p µ4y
u p µ2z
µ2z
R2

| β| ∈ [75, 90]◦
θ̂
NRMS (%)
3.34E-04
3.931
3.99E-03
3.751
-2.24E-02
3.192
1.34E-01
2.683
1.35E+00
2.456
2.20E+01
2.350
-5.43E-02
2.187
-4.51E+01
2.159
-4.36E+00
2.129
-2.81E-03
2.087
4.01E-02
2.061
-1.31E+02
2.047
5.11E+00
2.008
-3.73E-01
1.994
-1.68E-01
1.975
-2.23E-02
1.962
0.7476

Table 12

Estimation result of Cn model, µ ≤ 0.05

reg.
1
s(ur )ur2
r
ur
µ2x r
R2

Table 13

reg.
1
ur
ur µx
µ y µz
s(r)r̄ 2
R2

| β| ∈ [0, 30]◦
θ̂
NRMS (%)
5.470E-05
7.245
6.976E-04
3.446
-5.354E-04
3.353
4.554E-02
3.276
-1.781E+00
3.244
0.8060

θ̂
NRMS (%)
2.188E-05
13.930
2.032E-04
3.382
-3.317E-02
2.689
3.099E-04
2.492
3.271E+02
2.460
0.9690

Estimation results of Cn model, µ > 0.05

reg.
1
ur3
ur µx
µx µ3y
µ3y
s(ur )ur2
µx µx
s(µy )µ2y µz
ur µz
s(r̄)r̄ 2 ur
s(ur )ur µz
µy µ2z
R2

| β| ∈ [30, 60]◦
θ̂
NRMS (%)
1.350E-05
7.120
-4.156E-05
5.883
-3.286E-04
5.471
-2.113E-01
5.368
-2.595E-02
5.100
3.005E-04
4.485
1.349E-02
4.413
2.348E-01
4.335
-4.223E-03
4.245
5.652E+01
4.188
1.287E-03
4.156
1.188E-01
4.141
0.6652

35

reg.
1
ur3
µ x µy
s(ur )ur2
µy
ur
s(r̄)r̄ 2
µ x µ y µz
R2

| β| ∈ [60, 90]◦
θ̂
NRMS (%)
2.202E-05
8.291
3.320E-05
6.842
-4.661E-02
6.280
-7.975E-05
6.170
-1.640E-03
5.702
4.716E-04
5.445
-2.994E+00
5.366
-1.210E-01
5.352
0.5859

References
[1] Mellinger, D., Michael, N., and Kumar, V., “Trajectory Generation and Control For Precise Aggressive Maneuvers with
Quadrotors,” Springer Tracts in Advanced Robotics, Vol. 79, 2014, pp. 361–373. doi:10.1007/978-3-642-28572-1_25.
[2] Hehn, M., and Dandrea, R., “Real-Time Trajectory Generation for Quadrocopters,” IEEE Transactions on Robotics, Vol. 31,
No. 4, 2015, pp. 877–892. doi:10.1109/TRO.2015.2432611.
[3] Smeur, E. J., De Croon, G. C., and Chu, Q., “Gust Disturbance Alleviation with Incremental Nonlinear Dynamic Inversion,”
IEEE International Conference on Intelligent Robots and Systems, Vol. 2016-Novem, 2016, pp. 5626–5631. doi:10.1109/IROS.
2016.7759827.
[4] Huang, H., Hoffmann, G. M., Waslander, S. L., and Tomlin, C. J., “Aerodynamics and control of autonomous quadrotor
helicopters in aggressive maneuvering,” Proceedings - IEEE International Conference on Robotics and Automation, 2009, pp.
3277–3282. doi:10.1109/ROBOT.2009.5152561.
[5] Alexis, K., Nikolakopoulos, G., and Tzes, A., “Switching Model Predictive Attitude Control for a Quadrotor Helicopter Subject
to Atmospheric Disturbances,” Control Engineering Practice, Vol. 19, No. 10, 2011, pp. 1195–1207. doi:10.1016/j.conengprac.
2011.06.010.
[6] Leishman, R. C., MacDonald, J. C., Beard, R. W., and McLain, T. W., “Quadrotors and Accelerometers: State Estimation with
an Improved Dynamic Model,” IEEE Control Systems, Vol. 34, No. 1, 2014, pp. 28–41. doi:10.1109/MCS.2013.2287362.
[7] Carroll, T., George, I.-R. E., and Bramesfeld, G., “Design Optimization of Small Rotors in Quad-Rotor Configuration,” 54th
AIAA Aerospace Sciences Meeting, 2016. doi:10.2514/6.2016-1788.
[8] Foster, J. V., and Hartman, D., “High-Fidelity Multi-Rotor Unmanned Aircraft System (UAS) Simulation Development for
Trajectory Prediction Under Off-Nominal Flight Dynamics,” 17th AIAA Aviation Technology, Integration, and Operations
Conference, 2017. doi:10.2514/6.2017-3271.
[9] Zhang, Y., de Visser, C. C., and Chu, Q. P., “Aircraft Damage Identification and Classification for Database-Driven
Online Flight-Envelope Prediction,” Journal of Guidance, Control, and Dynamics, Vol. 41, No. 2, 2017, pp. 449–460.
doi:10.2514/1.G002866.
[10] Hoffmann, G. M., Huang, H., Waslander, S. L., and Tomlin, C. J., “Quadrotor Helicopter Flight Dynamics and Control
: Theory and Experiment,” American Institute of Aeronautics and Astronautics, Vol. 4, No. August, 2007, pp. 1–20.
doi:10.2514/6.2007-6461.
[11] Hoffmann, G. M., Huang, H., Waslander, S. L., and Tomlin, C. J., “Precision Flight Control for a Multi-Vehicle Quadrotor
Helicopter Testbed,” Control Engineering Practice, Vol. 19, No. 9, 2011, pp. 1023–1036. doi:10.1016/j.conengprac.2011.04.005.
[12] Martin, P., and Salaün, E., “The True Role of Accelerometer Feedback in Quadrotor Control,” Proceedings - IEEE International
Conference on Robotics and Automation, 2010, pp. 1623–1629. doi:10.1109/ROBOT.2010.5509980.

36

[13] Kaya, D., and Kutay, A. T., “Aerodynamic Modeling and Parameter Estimation of a Quadrotor Helicopter,” AIAA Atmospheric
Flight Mechanics Conference, 2014. doi:10.2514/6.2014-2558.
[14] Orsag, M., and Bog, S., “Influence of Forward and Descent Flight on Quadrotor Dynamics,” Recent Advances in Aircraft
Technology, InTech, 2012, pp. 141–156. doi:10.5772/37438.
[15] Tang, Y. R., and Li, Y., “Dynamic Modeling for High-Performance Controller Design of a UAV Quadrotor,” 2015 IEEE
International Conference on Information and Automation, 2015, pp. 3112–3117. doi:10.1109/ICInfA.2015.7279823.
[16] Schulz, M., Augugliaro, F., Ritz, R., and D’Andrea, R., “High-Speed, Steady Flight with a Quadrocopter in a Confined
Environment Using a Tether,” IEEE International Conference on Intelligent Robots and Systems, Vol. 2015-Decem, 2015, pp.
1279–1284. doi:10.1109/IROS.2015.7353533.
[17] Gill, R., and Andrea, R. D., “Propeller Thrust and Drag in Forward Flight,” IEEE Conference on Control Technology and
Applications (CCTA), Mauna Lani, HI, 2017, pp. 73–79. doi:10.1109/CCTA.2017.8062443.
[18] Khan, W., and Nahon, M., “Toward an Accurate Physics-based UAV Thruster Model,” IEEE/ASME Transactions on Mechatronics,
Vol. 18, No. 4, 2013, pp. 1269–1279. doi:10.1109/TMECH.2013.2264105.
[19] Mahony, R., Kumar, V., and Corke, P., “Multirotor Aerial Vehicles: Modeling, Estimation, and Control of Quadrotor,” IEEE
Robotics & Automation Magazine, Vol. 19, No. 3, 2012, pp. 20–32. doi:10.1109/MRA.2012.2206474.
[20] Bristeau, P.-j., Martin, P., Salaun, E., and Petit, N., “The role of propeller aerodynamics in the model of a quadrotor UAV,”
2009 European Control Conference (ECC), IEEE, 2009, pp. 683–688. doi:10.23919/ECC.2009.7074482.
[21] Pounds, P., Mahony, R., and Corke, P., “Modelling and Control of a Large Quadrotor Robot,” Control Engineering Practice,
Vol. 18, No. 7, 2010, pp. 691–699. doi:10.1016/j.conengprac.2010.02.008.
[22] Russell, C., Jung, J., Willink, G., and Glasner, B., “Wind Tunnel and Hover Performance Test Results for Multicopter UAS
Vehicles,” American Helicopter Society 72nd Annual Forum., West Palm Beach, FL, 2016.
[23] Luo, J., Zhu, L., and Yan, G., “Novel Quadrotor Forward-Flight Model Based on Wake Interference,” AIAA Journal, Vol. 53,
No. 12, 2015, pp. 3522–3533. doi:10.2514/1.J053011.
[24] Morelli, E. A., “Global Nonlinear Aerodynamic Modeling Using Multivariate Orthogonal Functions,” Journal of Aircraft,
Vol. 32, No. 2, 1995, pp. 270–277. doi:10.2514/3.46712.
[25] Klein, V., and Morelli, E. A., Aircraft System Identification: Theory and Practice, AIAA, Blacksburg, VA, 2006. doi:
10.2514/4.861505.
[26] Lombaerts, T., “Fault Tolerant Flight Control: A Physical Model Approach,” Ph.D. thesis, Delft Univ. of Technology, Delft,
The Netherlands, 2010.

37

[27] Armanini, S. F., Karásek, M., de Croon, G. C. H. E., and de Visser, C. C., “Onboard/Offboard Sensor Fusion for High-Fidelity
Flapping-Wing Robot Flight Data,” Journal of Guidance, Control, and Dynamics, Vol. 40, No. 8, 2017, pp. 2121–2132.
doi:10.2514/1.G002527.
[28] Smeur, E. J. J., Chu, Q., and Guido C. H. E. de Croon, “Adaptive Incremental Nonlinear Dynamic Inversion for Attitude
Control of Micro Aerial Vehicles,” Journal of Guidance, Control, and Dynamics, Vol. 39, No. 3, 2016, pp. 450–461.
doi:10.2514/1.G001490.
[29] “Paparazzi open-source unmanned (air) vehicle project,” , Apr 2018. URL https://github.com/paparazzi/paparazzi.
[30] Mendes, A., van Kampen, E., Remes, B., and Chu, Q. P., “Determining moments of inertia of small UAVs: A comparative
analysis of an experimental method versus theoretical approaches,” AIAA Guidance, Navigation, and Control Conference,
American Institute of Aeronautics and Astronautics, Reston, Virigina, 2012. doi:10.2514/6.2012-4463.
[31] Powers, C., Mellinger, D., Kushleyev, A., Kothmann, B., Kumar, V., Influence of Aerodynamics and Proximity Effects
in Quadrotor Flight, Springer Tracts in Advanced Robotics, Springer International Publishing, Heidelberg, 2013. doi:
10.1007/978-3-319-00065-7.

38

