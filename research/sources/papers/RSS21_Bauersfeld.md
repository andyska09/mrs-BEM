     This paper has been accepted for publication at Robotics: Science and Systems 2021 conference.



NeuroBEM: Hybrid Aerodynamic Quadrotor Model
                   Leonard Bauersfeld∗ , Elia Kaufmann∗ , Philipp Foehn, Sihao Sun, Davide Scaramuzza




Fig. 1: Long-exposure images depicting quadrotor trajectory tracking at speeds up to 65 km/h in a large-scale motion-capture system. The captured data is
used to fit a hybrid quadrotor model combining blade-element-momentum (BEM) theory with a neural network compensating residual dynamics. This hybrid
model reproduces the flown trajectories in simulation with a positional RMSE error reduction of over 50% compared to state-of-the-art.

   Abstract—Quadrotors are extremely agile, so much in fact, that                           Our Approach                             First Principles
classic first-principle-models come to their limits. Aerodynamic
effects, while insignificant at low speeds, become the dominant                             
                                                                                      Ωk,cmd                      MM                RM
model defect during high speeds or agile maneuvers. Accurate                                                                                  fprop
modeling is needed to design robust high-performance control                                     
                                                                                   xk       Ωk                                                τprop
systems and enable flying close to the platform’s physical limits.              x
                                                                                 k−1      Ωk−1 
We propose a hybrid approach fusing first principles and                                        
learning to model quadrotors and their aerodynamic effects with                  xk−2     Ωk−2                                                      + f
                                                                                                
                                                                                 .                                                                     τ
unprecedented accuracy. First principles fail to capture such                    .          .. 
aerodynamic effects, rendering traditional approaches inaccurate
                                                                                 .
                                                                                 .           .                      NN
                                                                                 .           .. 
                                                                                                 
when used for simulation or controller tuning. Data-driven                       .            .                                          fres
approaches try to capture aerodynamic effects with blackbox                                                                                τres
                                                                                  xk−h     Ωk−h
modeling, such as neural networks; however, they struggle to                                                                        Learning Based
robustly generalize to arbitrary flight conditions. Our hybrid
approach unifies and outperforms both first-principles blade-                  Fig. 2: Overview of the proposed architecture to predict aerodynamic forces
element momentum theory and learned residual dynamics. It is                   and torques. The physical modeling pipeline (upper part) consists of a motor
evaluated in one of the world’s largest motion-capture systems,                model (MM) and a rotor model (RM)—detailed in Sections III-C and III-D.
using autonomous-quadrotor-flight data at speeds up to 65 km/h.                It takes the current state xk , current motor speeds Ωk , and the motor speed
The resulting model captures the aerodynamic thrust, torques,                  command Ωk,cmd as an input. Combined with the estimate of the residual
and parasitic effects with astonishing accuracy, outperforming                 forces and torques predicted by the neural network (NN) using the current
existing models with 50% reduced prediction errors, and shows                  and past h states, the acting force f and torque τ are calculated.
strong generalization capabilities beyond the training set.
                                                                                  Accurately modeling quadrotors flying at their physical lim-
                   S UPPLEMENTARY M ATERIAL                                    its is extremely challenging and requires to capture complex
   A narrated video illustrating our approach is available at                  effects due to aerodynamic forces, motor dynamics, and vibra-
https://youtu.be/Nze1wlfmzTQ. Code and dataset can be found                    tions. Especially aerodynamic forces pose a challenge, as they
at http://rpg.ifi.uzh.ch/NeuroBEM.html.                                        depend on hidden state variables like airflow, which cannot
                                                                               be easily measured. Furthermore, the individual downwash
                       I. I NTRODUCTION
                                                                               induced by the rotors interacts with both the frame and the
   In recent years, research on fast navigation of autonomous                  blades depending on the current state of the platform. The
quadrotors has made tremendous progress, continually pushing                   repeatability of tracking errors observed in prior work [1, 8, 9]
the vehicles to more aggressive maneuvers [1–4] (Figure 1).                    and in this work when performing aggressive maneuvers
To further advance the field, several competitions have been                   suggests that the difficulty of learning quadrotor dynamics
organized, such as the autonomous drone race series at the                     is not caused by stochasticity in the dynamics, but rather by
recent IROS and NeurIPS conferences [5, 6] and the AlphaPi-                    unobserved state variables such as airflow.
lot challenge [7]. In the near future, estimation and control                     Traditional approaches to quadrotor modeling limit the
algorithms will reach the level of maturity necessary to push                  captured effects to simple linear drag approximations and
autonomous quadrotors to the bounds of what is physically                      quadratic thrust curves [10–12]. Such approximations are
possible. This presents the need for quadrotor models that                     computationally efficient and describe the platform well in
can predict the behaviour of the platform even during highly                   low-speed regimes, but exhibit increasing bias at higher ve-
aggressive maneuvers.                                                          locities as they neglect the influence of the inflow velocity
∗ Equal contribution
                                                                               on the generated thrust. More elaborate models based on
The authors are with the Robotics and Perception Group, Dep. of Informatics,
University of Zurich, and Dep. of Neuroinformatics, University of Zurich and   blade-element-momentum (BEM) theory manage to accurately
ETH Zurich, Switzerland (http://rpg.ifi.uzh.ch).                               model single rotors at high wind velocities, but they do not
account for the aerodynamic interactions between rotors and         flight, they do neither account for the case where the rotors
the frame. Parametric gray-box models [13] aim to overcome          move through air, nor for rotor-to-rotor and rotor-to-body
these limitations by describing the forces and torques as a         interactions. Nevertheless, due to its simplicity, this model
linear combination of library functions. While these models         is still used in well-known aerial robotics simulators such as
can perform well, their performance hinges on the appropriate       AirSim [11], Flightmare [19], RotorS [10] and others [20].
choice of basis functions, which require human expert knowl-        To improve the accuracy of the thrust model in non-stationary
edge to design. Recent research has investigated computational      flights over the simple quadratic model, momentum theory
fluid dynamics [14] to model the aerodynamic effects at play        has been used in [21–23]. Blade element theory is another
during different flight conditions. While being very accurate,      approach to model a single rotor more accurately. The forces
such approaches are computationally expensive and need hours        and torques acting on each infinitesimal portion of the blade
of processing on a compute cluster, rendering them impractical      are integrated over the whole propeller [24]. This theory has
for experiments spanning more than a few seconds.                   been adopted to model aerodynamic effects on a quadrotor
   Accurately predicting forces acting on the quadrotor at          in many studies [25–29]. However, both blade-element theory
high speeds requires to implicitly estimate the airflow around      and momentum theory require the value of the induced veloc-
the vehicle. Although this state variable cannot be directly        ity which is challenging to estimate. Hence, the blade-element-
observed, it can be deduced from a sequence of measure-             momentum (BEM) theory is proposed, which combines the
ments of other observable state variables. Thus, learning a         above two theories to alleviate the difficulty of calculating the
high-order dynamics model requires a method for regression          induced velocity. The resulting model can accurately capture
of a nonlinear function in a high-dimensional input space.          aerodynamic forces and torques acting on single rotors in a
Deep neural networks have shown to excel at such high-              wide range of operating conditions [30–32].
dimensional regression tasks and have already been applied             Even though BEM outperforms simple quadratic models
to dynamic system modeling [8, 9, 15–17]. Despite showing           and often achieves accurate predictions, it does not account for
promising performance, such purely-learned models require           any interaction between the flow tubes of different propellers
large amounts of data and require careful regularization to         or the frame [13]. Previous work has incorporated interaction
avoid overfitting.                                                  effects using either static wind tunnel tests [33–35] where the
Contribution                                                        vehicle is rigidly mounted on a force sensor, or by performing
                                                                    fast maneuvers in instrumented tracking volumes [36]. In [36]
   This work proposes a quadrotor dynamics model that can
                                                                    a simple quadratic model is combined with residual forces
accurately capture complex aerodynamic effects by combining
                                                                    predicted by Gaussian Processes. While this approach offers a
a state-of-the-art rotor model based on BEM theory with
                                                                    lightweight solution to learn residual forces and can be used
learned residual force and torque terms represented by a
                                                                    for control, it does not model residual torques, effectively ne-
deep neural network. The resulting hybrid model benefits
                                                                    glecting moments caused by rotor-to-rotor interactions. In [13],
from the expressive power of deep neural networks and the
                                                                    the quadrotor platform is identified using a gray-box model
generalizability of first-principles modeling. The latter reduces
                                                                    that uses a library of polynomials as basis functions and is able
the need for extreme amounts of training data. The model is
                                                                    to model both aerodynamic forces and torques. This method
identified using data collected from a large set of maneuvers
                                                                    relies on the predefined function library and also contains
performed on a real quadrotor platform. Leveraging one of the
                                                                    discontinuities in the learned model. Another line of works
biggest optical tracking volumes in the world, the platform’s
                                                                    investigates the modeling of quadrotors using computational
state as well as the motor speeds are recorded during flight.
                                                                    fluid dynamics (CFD) [14, 37]. While such simulations achieve
The resulting dataset contains 96 flights with a cumulative time
                                                                    results that are highly accurate and manage to capture real-
of 1h 15min and 1.8 million data points, covering the entire
                                                                    world effects well, they require large amounts of computation
performance envelope of the platform up to observed speeds
                                                                    time on high-performance compute clusters.
of 65 km h−1 (18 m s−1 ) and accelerations of 46.8 m s−2 .
                                                                       Due to their ability to identify patterns in large amounts
   The proposed model is compared against state-of-the-art
                                                                    of data, deep neural networks represent a promising approach
modeling approaches on unseen test maneuvers. The com-
                                                                    to model aerodynamic effects precisely and computationally
parison is done in terms of both evaluation of predicted
                                                                    efficient. A recent line of works employs deep neural networks
aerodynamic forces and torques and closed-loop integration
                                                                    to learn quadrotor dynamics model purely from data, for
of the model in a simulator, each evaluated against real-world
                                                                    both continuous time formulations [8, 9] as well as discrete-
reference data. In both categories, a performance increase by
                                                                    time formulations [15, 16, 38, 39]. While approaches relying
a factor of two is observed.
                                                                    entirely on learning-based methods have high representative
                      II. R ELATED W ORK                            power and the potential to also learn complex interaction
   Traditionally, a rotor is assumed to produce thrust and axial    effects, they require large amounts of data to train and careful
torque proportional to the square of its angular rate with          regularization to avoid overfitting.
a constant coefficient [18], which is referred to hereinafter          The approach presented in this work is inspired by [8, 9],
as the simple quadratic model. While these assumptions are          but instead of learning the full dynamics, it combines state-of-
valid for rotors on a static thrust stand and for near-hover        the-art BEM modeling based on first principles with a data-
                                                                             where gW = [0, 0, −9.81 m/s2 ]| denotes earth’s gravity, fprop
               3                                                             is the collective force produced by the propellers including
                                                                             any parasitic effects the rotor model can simulate (e.g. induced
                               yB zB                    4                    drag), and fres denotes residual forces that are not explained
                                                                             by the rotor model used. Similarly, τprop and τres are the
     zW                                            xB                        cumulative torques acting on the platform due to the propellers
                              1
              yW                                                             and residual torques that are not explained by the rotor model.
                                         gW                     2
                                                                                                        X
                                                                                               fprop =       fi                            (2)
                       xW                                                                                i
Fig. 3: Diagram of the quadrotor model depicting the world and body frames                               X
and illustrating the propeller numbering convention.                                           τprop =       τi + rP,i × fi ,               (3)
                                                                                                         i
driven approach to learn the residual force and torque terms.                where rP,i is the location of propeller i expressed in the body
The resulting model benefits from the strong generalization                  frame and fi , τi are the forces and torques generated by the
performance of traditional first-principle modeling and the                  i-th propeller. The rotor models aim at predicting accurate
flexibility of learning-based function approximation.                        estimates of the single-rotor forces and torques fi , τi , as
                  III. Q UADROTOR M ODEL                                     explained in the following sections. The force and torque
                                                                             effects of the fuselage, body, and rotor interaction are not
   This section explains the hybrid quadrotor model proposed                 explicitly modeled, but should be captured by the residual
in this work. It starts by introducing the notation and the                  dynamics fres and τres , predicted by a neural network.
rigid body dynamics (Figure 3), proceeds to explaining two
approaches to single-rotor modeling of increasing complexity                 C. Rotor Model: Quadratic
and concludes with the learned residual model. The hybrid                       The simplest model for single propeller is a quadratic fit
structure of the model, illustrated in Figure 2, consists of a               which assumes the thrust and torque produced by a single
rotor model and a learned correction.                                        propeller to be proportional to the square of its rotational rate
A. Notation                                                                  (propeller speed) Ω.
                                                                                                                                
   Scalars are denoted in non-bold [s, S], vectors in low-                                             0                     0
ercase bold v, and matrices in uppercase bold M . World                                fi (Ω) =  0  τi (Ω) =  0                        (4)
W and Body B frames are defined with orthonormal basis                                            cl,q · Ω2              cd,q · Ω2
i.e. {xW , yW , zW }. The frame B is located at the center                   The coefficients cl,q and cd,q are typically identified using a
of mass of the quadrotor. A vector from coordinate p1 to                     static propeller test stand. This simplified model is a good ap-
p2 expressed in the W frame is written as: W v12 . If the                    proximation for near-hover flight at near-zero velocity without
vector’s origin coincides with the frame it is described in, the             ceiling or ground effects [29] and explains static thrust-test
frame index is dropped, e.g. the quadrotor position is denoted               stand measurements very well. However, it ignores that ego-
as pWB . Furthermore, unit quaternions q = (qw , qx , qy , qz )              motion impacts the lift generated by the propeller. The induced
with kqk = 1 are used to represent orientations, such as                     drag, which depends on the propeller speed and body-relative
the attitude state of the quadrotor body qWB . Finally, full                 air velocity, is neglected as well, albeit being the dominant
SE3 transformations, such as changing the frame of reference                 source of drag for quadrotors. This is sometimes mitigated
from body to world for a point pB1 , can be described by                     by combining the model with a linear drag term such as
W pB1 = W tWB + qWB          pB1 . Note the quaternion-vector                in [10, 12].
product is denoted by representing a rotation of the vector
by the quaternion as in q        v = qv q̄, where q̄ is the                  D. Rotor Model: BEM
quaternion’s conjugate.                                                         Compared to the quadratic model, Blade-Element-
                                                                             Momentum-Theory (BEM) accounts for the effects of varying
B. Quadrotor Dynamics                                                        relative air speed on the rotor thrust. It assumes interaction
   The quadrotor is assumed to be a 6 degree-of-freedom rigid                effects between individual rotors to be negligible and
body of mass m and diagonal moment of inertia matrix J =                     describes each rotor separately. The approach presented here
diag(Jx , Jy , Jz ). The state space is thus 13-dimensional and              is based on classical propeller modeling for helicopters [24].
its dynamics can be written as:                                              The modeling of the propeller lift and drag coefficients is
                                     v           W
                                                      
                                                                            based on [31, 40].
                                  0
                      qWB ·                                                 First, basic momentum theory is introduced and used to re-
          ṗWB                  ωB /2
                                                                             late the thrust force to a given velocity difference in a flow-tube
                                               
                                             
         q̇WB   1 qWB                
    ẋ = v̇  =  m       fprop + fres    + gW 
                                                 ,                   (1)    across the rotor. Then the aerodynamic blade-element model
            WB             | {z }
                                                                             is presented. While momentum theory uses the momentum
                                               
           ω̇B                :=f
                  J −1 τ + τ −ω × Jω  
                                                
                                  |
                                      prop
                                             {z
                                                  res
                                                   }
                                                        B       B            conservation to relate induced airspeed and generated thrust,
                                         :=τ                                 a blade-element model sums the contributions of infinitesimal
blade elements to the total thrust force and drag torque. Finally,
the full algorithm combining momentum theory and the blade-                               xP
element model is presented.                                                                                  vver
   For now, the induced velocity vi is considered to be known                                      a1
as momentum theory and the blade element model jointly yield
this information. Together with the known ego-motion of the                                                                                  yP
quadcopter, this fully determines the wind field around the                                                                      b1
individual propellers.                                                                             Ψ

Momentum Theory. The most simple theory for analyzing                                                                   vhor
rotors is momentum theory as it allows to calculate the thrust                                          vi
of a propeller based on a momentum balance across the rotor.
This balance is done inside a flow-tube with radius R that
fully contains the propeller. Assuming a known and constant                                             zP
induced velocity vi across the diameter of the flow tube, the        Fig. 4: Lateral flapping b1 and longitudinal flapping a1 occur due to lift
thrust T of a rotor is given by [24]:                                imbalance. The azimuth angle Ψ of the blade is measured ‘from the tail’ in
                                                                     the direction of rotation. The horizontal velocity vhor and vertical velocity
                                                                     vver are defined opposite to the propeller frame, i.e. if the propeller moves
                           q
               T = 2vi ρA vhor2 + (v            2
                                      ver − vi ) ,         (5)       along zP the relative velocity will also have a positive vertical component.
                                                                     Note that coning is not shown to improve the clarity of the schematic.
where vhor and vver denote the horizontal and vertical velocity
component of the flow tube. Note that momentum theory alone
does not provide any means for calculating the induced veloc-                                                          dT
                                                                                                                  dL
ity, it merely relates the induced velocity and the thrust based
on a momentum balance and does not make any assumption
on the physical process that actually accelerates the air.                          xP                        c
                                                                                                                            dD
                                                                                                   θ
Blade Element Theory. The main purpose of a blade element                       α             UT        ϕ
                                                                                                                               dH
model is to estimate the acting forces and torques accurately.                           UP                            vi
A propeller consists of b identical blades (typically b = 2                                        U
or b = 3) attached to the rotor hub, each acting as a wing                                                     zP
producing lift and drag forces. The propeller coordinate frame            Fig. 5: A blade element located at radius r and azimuth angle Ψ.
P shown in Figure 4 is defined such that the zP -axis points
down and the xP -axis opposes the horizontal component vhor            The tangential velocity UT and parallel-to-motor velocity
of the incoming wind.                                                UP are related to the angular velocity Ω of the propeller as
   The finite stiffness of the blade and its hub-mount allow it to
bend and deform, transmitting forces and introducing torques               UT (r, Ψ) = Ωr + vhor sin Ψ                                        (6)
around the hub. It also causes the rotor-disk plane to be tilted           UP (r, Ψ) = vver − vi                                              (7)
with respect to the propeller coordinate system. This defor-                             − rΩ(a1 sin Ψ + b1 cos Ψ)
mation can be split into a symmetric coning component a0                                 + vver (a0 − a1 cos Ψ − b1 sin Ψ) cos Ψ .
due to the overall lift produced by the propeller (not shown in
Figure 4) and an asymmetric flapping component that depends          The local angle of attack α can be calculated as
on the azimuth angle Ψ of the propeller. Figure 4 illustrates                   ϕ(r, Ψ) = arctan(UP (r, Ψ)/UT (r, Ψ))            (8)
this: in forward flight one side of the propeller experiences                                      r
                                                                                α(r, Ψ) = θ0 + θ1 + ϕ(r, Ψ) ,                    (9)
a higher relative airspeed (advancing blade) compared to the                                      R
opposite side (retracting blade). Thus, there is a lift imbalance    where θ0 is the pitch angle of the blade and θ1 the blade twist.
between the sides of the propeller which in turn causes the          The differential lift force dL and the differential drag dD can
elastic propeller to bend upwards on the advancing side. This        be expressed as functions of radius r and azimuth angle Ψ:
is called lateral flapping with the associated flapping angle b1 .
Due to the inertia of the blade, the lateral flapping also induces      dL(r, Ψ) = c(r)cl (α(r, Ψ))(UT (r, Ψ)2 + UP (r, Ψ)2 ) (10)
a longitudinal flapping angle a1 .                                     dD(r, Ψ) = c(r)cd (α(r, Ψ))(UT (r, Ψ)2 + UP (r, Ψ)2 ), (11)
   Figure 5 shows the velocities, angles and forces of a blade
element. The chord of the airfoil is rotated by an angle θ           where c(r) is the chord length and cl (α), cd (α) are the angle-
relative to the xy-plane. Together with the inflow angle ϕ,          of-attack dependent coefficients of lift and drag respectively.
this results in a total angle of attack α = θ + ϕ. Each blade        The coefficients are modeled as proposed in [31, 40] as
element produces a lift force dL perpendicular to the incoming          cd (α) = cd,0 sin2 α                 cl (α) = cl,0 sin α cos α ,     (12)
airstream and a drag force dD in the direction of the incoming
airflow. The thrust force dT and horizontal force dH are             where cd,0 and cl,0 are experimentally determined by measur-
aligned with the propeller coordinate frame.                         ing the lift and drag torque on a thrust test stand.
                  e                                                      Due to the nature of the physical modeling process, the
                                                                      results from the momentum theory are only valid if the vehicle
                               Mspring                                does not fly in its own downwash. This occurs when the
                                                                      vehicle descends with a certain speed, e.g. the propeller is
             Fig. 6: Illustration of the hinged blade model.          in vortex-ring state [21] if and only if
   The overall thrust T , horizontal force H and drag torque Q                                      vP,z
                                                                                              0<         <2.                    (18)
are obtained through integration.                                                                    vi
       bρ R 2π
           Z Z                                                        If the results from momentum-theory are not applicable, the
  T =                dL cos φ + dD sin φ dΨ dr             (13)       induced velocity can not be calculated. In [21] a solution is
       4π 0 0
                                                                      presented which relies on an empirical fit to calculate the
       bρ R 2π
           Z Z
 H=                 (−dL sin φ + dD cos φ) sin Ψ dΨ dr (14)           induced velocity in such flight conditions. The proposed fit
       4π 0 0                                                         consists of a quartic polynomial to approximate vi as follows:
       bρ R 2π
           Z Z
  Q=                (−dL sin φ + dD cos φ) r dΨ dr         (15)         ṽi = vh,i 1 + 1.125(vP,z /vh,i ) − 1.372(vP,z /vh,i )2   (19)
       4π 0 0
                                                                            +1.718(vP,z /vh,i )3 − 0.655(vP,z /vh,i )4 ,
                                                                                                                      
Blade Elasticity. Standard helicopters have their rotor blades
connected to the rotor hub through a hinge pin, optionally            where vh,i is the induced velocity if the vehicle would fly
with an offset. This is not true for small rotor sizes typically      horizontally in the given flight state, i.e. set vP,z = 0. To
found on multicopters, since the blade is fixed and elastic           ensure a smooth transition back to the physical modeling,
which breaks the assumptions made in standard helicopter              the final induced velocity in vortex ring state is given as
literature. Therefore, the model is slightly adapted to capture       vi = max(ṽi , vh,i ).
the characteristics of small propellers: a blade is rigid and         The algorithm thus consists of the following steps:
connected to the rotor hub with a hinge and a torsional spring
                                                                         1) Assume a0 = 0, a1 = 0, b1 = 0.
at an offset e [21] as shown in Figure 6. The coning angle a0
                                                                         2) Find vi such that (5) and (13) are simultaneously satis-
as well as the flapping angles a1 and b1 can be calculated by
                                                                            fied, i.e. the thrust calculated by momentum theory and
equating the moments acting on the rotor hub. At the hinge
                                                                            blade element theory are identical. If inside vortex-ring
position, the following moment equilibrium occurs:
                                                                            state, use the approximation presented above.
  0 = Mw + Mgyro + Minertial + Mcf + Maero + Mspring , (16)              3) Calculate the coning and flapping angle a0 , a1 and b1
where the moment Mw is caused by the weight of the blade,                   with the previously computed induced velocity.
the moment Mgyro is due to gyroscopic effects the blade                  4) Using the previously calculated induced velocity and
experiences when a non-zero rollrate or pitchrate are present,              blade flapping angles, (13) – (15) can be evaluated again.
Minertial comes from the inertia of the blade and its angular            5) The total force of the propeller and torque around the
acceleration during the flapping motion, Mcf is caused by                   center of the propeller are given by:
centrifugal forces when the blade flaps, the moment Maero is                            
                                                                                          −(H + sin a1 T )
                                                                                                                     
                                                                                                                        ±kβ b1
                                                                                                                               
a result of the lift generated by the blade, and lastly Mspring                 fP =  ± sin b1 T              τP =  kβ a1  ,
is the restoring moment produced by the hinge spring. For                                     −T cos a0                  ∓Q
brevity, the derivation of the coning and flapping angles are
omitted here. They closely follow [24] (pp. 463). Due to the                where the upper sign of ± and ∓ corresponds to
torsional spring at the hinge, the spring moment needs to be                propeller rotating clockwise and the lower sign needs
considered additionally:                                                    to be used for a counter-clockwise spinning propeller.
          Mspring = kβ (a0 + a1 cos Ψ + b1 sin Ψ) ,            (17)   E. Learned Residual Dynamics
where kβ is the given spring stiffness. From (16), the coning            Both rotor models presented in Sections III-C and III-D do
and flapping angles are calculated. The resulting expression is       not account for aerodynamic forces and torques caused by the
omitted here for readability.                                         quadrotor body or interaction effects between the propellers. In
Complete BEM-Model Algorithm. Throughout above ex-                    this work, these residual dynamics are approximated by a deep
planations, the induced velocity vi was treated as a known            neural network. Modeling such effects accurately requires to
quantity. However, when simulating the vehicle the induced            implicitly estimate the airflow around the vehicle. Considering
velocity is unknown and needs to be calculated. This can              the airflow as hidden state of the system, it can be estimated
be done by combining the results from momentum theory                 by measuring a history of observable state variables. In this
and blade-element theory: (5) and (13) can both be used to            work, the angular and linear velocities, as well as motor speeds
calculate the thrust of the propeller. Due to the relatively stiff    are used as input features for the neural network. A history
blade, the flapping and coning angle are small (typically less        length of h = 20, with temporary equally-spaced samples with
than 1°) and can thus be neglected as a first approximation to        a δt = 2.5 ms is used, effectively giving information of the
calculate the induced velocity.                                       platform evolution over the past 50 ms.
TABLE I: Comparison of different network architectures with respect to   set. Each subsets contains trajectories that cover the full range
RMSE of force and torque prediction on a held-out test set.
                                                                         of speeds and accelerations observed in the full data set.
 Architecture       Force RMSE [N]    Torque RMSE [Nm]      # Param
 TCN small                   0.365       6.525 ×10−3          12k        B. Quadrotor Platform
 TCN medium                  0.352       5.274 ×10−3          25k
 TCN large                   0.355       4.674 ×10−3          72k           The real-world flights are performed with a custom-made
 MLP                         0.356       5.172 ×10−3          30k        quadrotor platform. It features an Armattan Chameleon 6 inch
                                                                         main frame, equipped with Hobbywing XRotor 2306 motors
   The network architecture is empirically validated by min-             and 5 inch, three-bladed propellers. The platform has a total
imizing the prediction error on an unseen test set. The can-             weight of 752 g and can produce a maximum static thrust
didate architectures consist of temporal-convolutional (TCN)             of approximately 33 N, which results in a static thrust-to-
encoders [41] and fully-connected (MLP) encoders, which                  weight ratio of 4.5. The weight and power of this platform
both are combined with two fully-connected heads, one for                is comparable to the ones used by professional pilots in drone
the residual force prediction and one for the residual torque            racing competitions. The platform’s main computational unit
prediction. Each architecture uses leaky-ReLU activations and            is an NVIDIA Jetson TX2 accompanied by a ConnectTech
a linear output layer. Training is performed in a supervised             Quasar carrier board. In all real world flights, control com-
fashion using the Adam optimizer by minimizing the RMSE                  mands in the form of collective thrust and bodyrates are
loss on forces and torques between predictions and labels.               computed on a laptop computer and sent via a Laird module
Table I shows the main results of these ablation experiments.            to the Jetson TX2. The Jetson then forwards these commands
Due to its favourable performance versus inference time trade            to a commercial flight controller running BetaFlight2 , which
off, the medium-sized temporal-convolutional encoder (TCN-               produces single-rotor commands that are fed to a 4-in-1
medium) was selected for all subsequent experiments.                     electronic speed controller.
                   IV. E XPERIMENTAL S ETUP
                                                                         C. Control System
A. Data Collection
                                                                            Control commands are produced by a control pipeline
   To train the model and to verify its accuracy, real world             consisting of two levels: (i) a high-level non-linear quadratic
measurement data is needed. It is recorded in a flying arena             MPC controller generating bodyrate and collective thrust com-
equipped with a motion tracking system with a usable volume              mands at 100 Hz, and (ii) a low-level Betaflight controller
of 25 m × 25 m × 8 m. The Vicon1 motion tracking system                  tracking the desired bodyrate setpoint at 1 kHz. To ensure
allows to record accurate position and attitude measurements at          repeatability, BetaFlight features targeted at human piloted
400 Hz. Additionally, onboard IMU measurements and motor                 drones (feed-forward terms) are disabled, and the controller
speeds are recorded at 1 kHz by the low-level flight controller.         is reduced to a PID with equal parameters for simulation
This onboard data and the pose measurements need to be                   and real-world flight. BetaFlight is run as a software module
synchronized and fused in post processing. For this purpose              within the simulation, resembling the real control system. Both
interpolating cubic splines are fitted to the datapoints, which          controllers are kept equal in the simulation with respect to
allows fusing the asynchronous measurements from both data               the real-world experiments, to guarantee equal performance in
sources, and recovers the full dynamic state. Furthermore,               both scenarios.
to estimate the unobserved linear velocity and angular ac-
celeration, differentiation of the fitted splines provides less          D. Simulator Extension
noisy estimates than direct differentiation of the discrete, noisy
                                                                            To compare different models, their resulting simulation
measurements. For the means of time synchronization, offset
                                                                         accuracy is evaluated with respect to the real-world trajectory.
and clock skew are estimated through the correlation quality
                                                                         For this purpose, the closed-loop system is simulated forward
of the axis-wise angular rate measurement from the IMU
                                                                         in time. While the rigid-body dynamics are given by (1) and
with the spline. Gyroscope measurements are used because
                                                                         the aerodynamic force and torque are provided by the model
they provide better noise characteristics than the accelerometer
                                                                         in question, there are two further components needed for an
data. The clock skew was typically observed to be 2.4 %. The
                                                                         accurate simulation: integration and motor dynamics.
motor data is smoothed with a finite-impulse-response fourth-
order Butterworth low-pass filter with a cutoff frequency                Integration. The integration is performed by a symplectic
corresponding to the time-constant of the motors, identified             Euler scheme with a timestep of 1 ms using the rigid body
from the step response of the motors. This ensures that noise            dynamics (1), the modeled linear and angular accelerations of
is suppressed without attenuating high-frequency motor signals           the tested model, and the motor dynamics explained in the fol-
more than 3 dB.                                                          lowing section. The advantage of the symplectic Euler scheme
   The resulting dataset contains 1.8 million data points                is its energy conservation property, which is invalidated with
recorded from 96 flights covering 1h:15min of flight time. The           other integration schemes, such as the standard Euler methods
dataset is split into 70% training, 20% validation, and 10% test         or the Runge-Kutta family of integrators.
  1 https://www.vicon.com/                                                 2 https://github.com/betaflight/betaflight
                                                                  TABLE II: Comparison of model performance in terms of RMSE on an unseen
Motor Dynamics. Since the aerodynamic model is based on           test set. Approaches marked with an asterisk are trained on a reduced training
the angular speed of the propeller, the motors are modeled as     set to compare generalization performance.
a first-order system according to
                                                                     Model                       Fxy        Fz             Mxy            Mz          F       M
                   δ       1                                                                     [N]        [N]            [Nm]          [Nm]        [N]     [Nm]
                     Ω=      (Ωcmd − Ω)                (20)
                  δt      τΩ                                         None                       1.549       13.618        0.036          0.006       7.964   0.029
                                                                     Fit                        1.536       1.381         0.104          0.033       1.486   0.087
where Ωcmd is the commanded propeller speed and τΩ is                BEM                        0.803       1.265         0.090          0.017       0.982   0.074
the motor time constant. For the quadrotor platform used             PolyFit [13]               0.453       0.832         0.027          0.008       0.606   0.022
throughout the experiments, the time constant was identified         None+NN                    0.236       0.681         0.017          0.002       0.438   0.014
                                                                     Fit+NN                     0.232       0.722         0.017          0.004       0.458   0.014
to be τΩ = 33 ms.                                                    BEM+NN (ours)              0.204       0.504         0.014          0.004       0.335   0.012
              V. E XPERIMENTS AND R ESULTS                           PolyFit* [13]              1.450       6.637         2.815          0.164       4.011   2.301
                                                                     None+NN*                   0.470       1.959         0.007          0.002       1.194   0.006
  The evaluation procedure is designed to address the fol-           Fit+NN*                    0.501       1.225         0.024          0.013       0.817   0.021
lowing questions: (i) When does the classical approach to            BEM+NN* (ours)             0.344       0.816         0.025          0.008       0.549   0.021
quadrotor modeling based on quadratic thrust and torque
curves start to break down? (ii) How do the forces and torques                           Measurement         BEM                     Graybox          BEM + NN
predicted by a model based on BEM compare with respect
                                                                                20                                              1
to a simple quadratic model? (iii) What is the contribution
of a learned residual dynamics component? The reader is                                                                        0.8
                                                                                15
encouraged to watch the attached video to understand the

                                                                  Speed [m/s]                                       Mxy [Nm]
                                                                                                                               0.6
highly dynamic nature of the experiments.                                       10
                                                                                                                               0.4
A. Experimental Setup
                                                                                5
                                                                                                                               0.2
   Throughout the experiments multiple models are fitted using
the training and validation data, and evaluated using the test                  0                                               0
                                                                                     0      2           4                            0           2           4
data. The predictive performance of these models is compared
in two different settings, covered in the subsequent sections:
                                                                                6                                              30
in Section V-B, the RMSE of predicted forces and torques
is compared on unseen flight data; in Section V-C, different                    4
                                                                      Fxy [N]                                         Fz [N]
                                                                                                                               20
dynamics models are evaluated in conjunction with a known
controller to determine the mismatch between simulation and                     2
                                                                                                                               10
the real world in a closed-loop scenario. Both types of compar-
isons are performed on unseen test trajectories that cover the                  0
entire performance envelope of the platform. Each trajectory                                                     0
                                                                                     0
                                                                                     2           4                 0          2          4
has been performed on the real platform in an instrumented                          Time [s]                                 Time [s]
tracking volume as explained in Section IV.                       Fig. 7: The plot illustrates the results presented in Table II. The plots show
                                                                  a highly aggressive maneuver (from the test dataset) where only models with
B. Comparison of Predictive Performance                           neural net augmentation predict the forces and torques well.
   In a first set of experiments, the presented models are
compared in terms of predicted forces and torques on unseen       in Section IV and a reduced dataset that only covers linear
trajectories. These trajectories cover the entire performance     speeds up to 5 m s−1 . This reduced dataset is used to identify
envelope of the quadrotor, ranging from slow near-hover           new parameters for each approach, which are marked with an
trajectories with speeds below 5 m s−1 to aggressive trajec-      asterisk.
tories at the limit of the platform’s capabilities, exceeding        Table II summarizes the results of this experiment, while
speeds of 18 m s−1 and accelerations up to 46.8 m s−2 . The       Figure 7 illustrates performance on a highly aggressive maneu-
models compared in this experiment consist of the quadratic       ver. The proposed hybrid model based on BEM and learned
model (Fit), the BEM model (BEM) and a naive model                residual dynamics consistently outperforms all other models
predicting all zeros (None). Each of these models is augmented    on the predicted forces. Note that the trajectories performed
with a learned residual correction using a neural network,        in this work are designed to minimize yaw rate, and as a result
marked with +NN. While the None model represents a naive          only cover extremely small yaw torques Mz (the largest yaw
baseline to better understand the magnitude of prediction er-     torque in the dataset is 0.072 N m), as discussed in Section
rors, None+NN illustrates the performance of a purely learned     VI. It is evident that thrust and torque along the body zB -axis
model. Finally, the approach presented in [13] is compared,       are more challenging to predict accurately. The reason for this
denoted as PolyFit as it uses automatically selected polynomial   is two-fold: First, all linear acceleration actuation lies in the
basis functions to fit the model.                                 zB direction, contributing actuation noise predominantly along
   To evaluate generalization performance, each approach is       this axis. Second, all trajectories are designed to minimize yaw
trained on two datasets: the entire training set as explained     torque, since this is the least actuated torque direction, and
TABLE III: Comparison of closed-loop simulation performance on an unseen test set of different trajectories. Results show the positional RMSE between
trajectories flown in simulation with different dynamics models and the same set of trajectories flown on the real platform. Models marked with an asterisk (? )
were trained only on slow data up to 5 m s−1 .



                         vmean [m/s]                                                     PolyFit [13]                                                                                                              RotorS [10]
                                                                                                                                                                                                    BEM+NN?
                                              vmax [m/s]                                                         None+NN                                             None+NN?
                                                                                                                                           BEM+NN
                                                                Fit           BEM                                               Fit+NN
                                                                                                                                           (ours)        PolyFit?                    Fit+NN?        (ours)

 Lemniscate             1.67                 3.51              0.061        0.059        0.043                  0.046          0.049       0.059     0.046          0.049           0.048           0.053         0.114
 Random Points          2.38                 8.25              0.167        0.138        0.130                  crash          0.126       0.141     crash          0.130           0.123           0.134         0.239
 Lemniscate             3.21                 7.04              0.183        0.146        0.102                  0.103          0.112       0.109     crash          0.100           0.112           0.112         0.148
 Melon                  3.57                 7.63              0.229        0.163        0.117                  0.126          0.126       0.133     crash          0.127           0.134           0.137         0.192
 Slanted Circle         6.92                10.75              0.381        0.232        0.166                  0.172          0.168       0.167     crash          0.210           0.193           0.185         0.257
 Linear Oscillation     7.25                16.95              0.506        0.438        0.172                  0.270          0.234       0.171     crash          0.258           0.263           0.216         0.562
 Race Track             7.64                13.14              0.414        0.286        0.233                  0.283          0.223       0.214     crash          0.257           0.262           0.240         0.424
 Melon                  7.74                13.55              0.431        0.221        0.239                  0.179          0.164       0.155     crash          0.263           0.207           0.185         0.398
 Slanted Circle         8.57                13.32              0.531        0.217        0.255                  0.206          0.197       0.192     crash          0.340           0.268           0.180         0.397
 Race Track             9.94                17.81              0.617        0.408        0.820                  0.447          0.320       0.301     crash          0.446           0.420           0.370         0.708
 Lemniscate             12.01               19.83              0.762        0.549        0.316                  0.782          0.352       0.286     crash          0.469           0.423           0.371         1.158
 Ellipse                15.02               19.20              0.855        0.369        crash                  0.347          0.402       0.285     crash          0.605           0.481           0.290         0.653

            Lemniscate, Ellipse                                            Race Track                                                    Melon                                   Random Points




        6                                                  6                                                               6                                         6
                                                                                                                                                                     4
Z [m]
        4                                                  4                                                               4
        2                                                  2                                                               2                                         2
        0                                       5          0                                                5              0                                  5      0                                            5
              0                           0                            0                                0                       0                    0                          0                             0
                                       −5                                                  −5                                                       −5                                         10         −5
             X [m]     10
                                            Y [m]                     X [m]         10
                                                                                                        Y [m]                  X [m]       10
                                                                                                                                                         Y [m]                  X [m]                          Y [m]

                                       Fig. 8: Trajectories used for testing. Each trajectory is flown multiple times with varying speeds.

significantly limits the acceleration envelope, and therefore the                                                      dimensions, lift and drag coefficients).
attainable agility. This explains the good performance of even                                                            Table III illustrates the results of the closed-loop exper-
very naive baselines with respect to this metric.                                                                      iment. For each trajectory, the accumulated positional error
   When comparing on the reduced dataset (Table II, bottom),                                                           between the simulated flight and the data observed in the real
the performance of the proposed approach gracefully degrades,                                                          world is reported. As can be seen, all models achieve similar
still outperforming the baselines in terms of predicted forces.                                                        performance for trajectories close to hover. The Fit model
Note that the PolyFit baseline completely breaks down in                                                               exhibits increasing bias with higher speeds, with the error
this setting, indicating poor generalization to unseen data.                                                           exceeding the worst performance of the proposed approach
The purely learning-based baseline outperforms all other ap-                                                           already at average speeds below 7 m s−1 . In contrast, the
proaches in the predicted torques, but also fails to generalize                                                        BEM model is able to maintain competitive performance up
the force predictions to the new data.                                                                                 to the fastest trajectories. At low speeds, the PolyFit baseline
                                                                                                                       performs very well, but exhibits increasing bias for higher
C. Closed-Loop Comparison                                                                                              speeds, even resulting in a crash on the fastest trajectory. The
   To demonstrate the benefits of an accurate force and torque                                                         RotorS baseline performs inferior on all trajectories, achieving
model, a second set of experiments presents a comparison                                                               results comparable to the Fit baseline. The proposed approach
of closed-loop simulation performance. Using the simulation                                                            combining BEM with a learned residual term (BEM+NN)
setup explained in Section IV-D, a set of unseen trajecto-                                                             achieves competitive performance on the slow trajectories and
ries (Figure 8) is flown in simulation and the resulting flight                                                        outperforms all baselines on the faster trajectories. Compared
path is compared with the data obtained from executing the                                                             to Fit+NN, BEM+NN achieves consistently better performance
same set of trajectories on the real platform. As for the                                                              for fast maneuvers.
previous set of experiments, also this comparison is performed                                                            When trained on the reduced dataset, all models show
for models identified on the full training set, as well as a                                                           decreased performance. However, while approaches such as
reduced training set to compare generalization performance.                                                            PolyFit completely break down, the proposed approach ex-
Additionally to the baselines already used in the previous ex-                                                         periences only a minor performance reduction around 20%,
periments, this experiment also compares against RotorS [10].                                                          outperforming the baselines on all faster trajectories. This
To do this, the standard model in RotorS is updated with the                                                           result highlights the ability of the proposed approach to
parameters identified from the real platform (i.e. mass, inertia,                                                      generalize beyond the data it was trained on (Figure 9).
                training set   ellipse test set    lemniscate test set            where most applications gladly trade-off real-time evaluation
                                                                                  for improved accuracy.
                                                                                     The results of the closed-loop simulation using the proposed
                                                                                  model could be further improved by refining the following
                                                                                  aspects of the control pipeline: (i) The experimental platform
                                                                                  currently relies on the BetaFlight inner-loop low-level con-
                                                                                  troller, which is optimized for human pilots. However, as such,
                                                                                  it only takes a throttle command and a body rate command as
                                                                                  inputs, and relies on the inner-loop to track the rate command.
                 10
                                                                                  Furthermore, it performs filtering and interpolation of the
                                                                         10
                                                                                  control signals to ensure a consistent flight feeling for human

   vz,B [m/s]
                  0                                                               pilots, which introduces undesirable control-loop shaping.
                                                                   0              An MPC outer-loop controller directly outputting single-rotor
                −10                                                               motor speeds that are tracked by an inner loop motor speed
                         −10                                                      controller would improve the accuracy of our approach further
                                     0                         −10 vy,B [m/s]     as this would minimize the differences between the simulation
                                                  10                              and the actual experiments. (ii) Modeling the latency from
                               vx,B [m/s]                                         the motion capture pose filtering, the data transmission to
Fig. 9: Visualization of the reduced training set, the ellipse test set and the   the drone, and the communication to the flight controller in
fastest lemniscate test set in the body-frame velocity space. Although the test
set mostly covers regions of the state space that are not part of the training
                                                                                  simulation would also reduce the error as it improves the
set, the trained BEM+NN? model still provides good accuracy in simulating         realism of the simulator. The authors expect the results in
the trajectory. This demonstrates its remarkable generalization capability.       Table III to be even more favorable for their approach in
                                                                                  such an ideal setting. The accuracy of the force and torque
                                                                                  predictions shown in Table II would also benefit from a custom
                                 VI. D ISCUSSION                                  low-level controller providing more precise and less noisy
   The results obtained in this work show that the proposed                       motor-speed information.
hybrid dynamics model, combining first-principles based on                           Compared to a purely learning-based approach such as
blade-element-momentum theory with a learning-based resid-                        None+NN, the proposed approach performs 25% better for
ual term, outperforms state-of-the-art modeling for quadrotors                    all non-trivial trajectories with average speeds above 4 m s−1 ,
with a 50% decreased aerodynamic force and torque prediction                      and extrapolates well to unseen flight data, as opposed to e.g.
error. Furthermore, evaluation in controlled experiments on                       the PolyFit baseline. The authors expect the performance of
a large real-world dataset shows that such a complementary                        learning-only approaches to improve with more data. However,
modeling approach outperforms each of its compositional                           in a real world setting, where high-quality data is sparse, pure
submodules.                                                                       learning-based approaches fall short of traditional methods.
   In fact, not only does the performance of the proposed hy-                     Additionally, accurate aerodynamic force and torque predic-
brid model structure improve with a more capable rotor model,                     tion does not necessarily translate to good closed-loop perfor-
but the learned residual dynamics also increase in accuracy if                    mance, as demonstrated in our evaluation. Moreover, this study
a broader envelope of effects can already be captured using                       observed cases where a purely-learned residual component
first principles. Specifically, the learned residual prediction                   introduced a feedback loop on the predicted torques that led
achieves up to 30% better performance when combined with                          to a crash. In such cases, support through first-principles is
a rotor model based on blade-element-momentum theory.                             vital for accurate and robust modeling.
   While the proposed approach significantly improves upon
state-of-the-art in quadrotor modeling for highly aggressive                                           VII. C ONCLUSION
maneuvers by up to 60%, its advantages for slow speed trajec-
tories below 5 m s−1 are limited. Our experiments indicate that                      This work proposes a novel method to model quadrotors by
for such slow trajectories, a traditional parametric approach                     combining modeling based on first principles with a learning-
such as [13] achieves very strong performance at a lower                          based residual term represented by a neural network. The
computational cost. While the quadratic and polynomial fits                       proposed method is able to accurately model quadrotors even
can be evaluated in a mere 1 µs, even on a micro processor,                       throughout aggressive trajectories pushing the platform to
the BEM model requires in the order of 100 µs on a modern                         its limits. This hybrid model outperforms its compositional
Intel-architecture CPU, and a forward pass of the network                         modules with up to 50% error reduction, including baseline
averages also at around 100 µs on a modern NVidia GPU.                            methods that utilize only first-principles modeling, as well as
The reason for the dominant runtime of the BEM model                              purely learning-based methods. The method shows strong gen-
is the necessary implicit solution for the induced velocity                       eralization beyond the training set used to identify the model
equation. Even though our approach is not optimized for                           and predicts accurate forces and torques where other methods
runtime, simulations can be run at an arbitrary timescale,                        break down. Controlled experiments indicate that the fusion of
learned dynamics with first-principles is a powerful combina-                         IEEE International Conference on Robotics and Automation (ICRA),
tion, where the learned dynamics-residual benefits from high-                         pages 2454–2459. IEEE, 2018.
                                                                                 [16] Gavin D Portwood, Peetak P Mitra, Mateus Dias Ribeiro, Tan Minh
fidelity models, such as the BEM. Applied to simulations, our                         Nguyen, Balasubramanya T Nadiga, Juan A Saenz, Michael Chertkov,
approach enables unprecedented accuracy, reducing positional                          Animesh Garg, Anima Anandkumar, and Andreas Dengel. Turbulence
RMSE from ∼0.8 m for state-of-the-art approaches, down to                             forecasting via neural ode. 2nd Workshop on Machine Learning and the
                                                                                      Physical Sciences (NeurIPS 2019), 2019.
below 0.3 m. This could tremendously speed up development                        [17] Radek Grzeszczuk, Demetri Terzopoulos, and Geoffrey Hinton. Neu-
and testing of advanced control and navigation strategies for                         roanimator: Fast neural network emulation and control of physics-based
quadrotors, without the need of the tedious and crash-prone                           models. In Proceedings of the 25th annual conference on Computer
                                                                                      graphics and interactive techniques, pages 9–20, 1998.
trial-and-error strategy on real systems.                                        [18] Robert Mahony, Vijay Kumar, and Peter Corke. Multirotor aerial
                                                                                      vehicles: Modeling, estimation, and control of quadrotor. IEEE Robotics
                        ACKNOWLEDGEMENT                                               and Automation magazine, 19(3):20–32, 2012.
   This work was supported by the National Centre of Com-                        [19] Yunlong Song, Selim Naji, Elia Kaufmann, Antonio Loquercio, and
                                                                                      Davide Scaramuzza. Flightmare: A flexible quadrotor simulator. 2020.
petence in Research (NCCR) Robotics through the Swiss                            [20] Johannes Meyer, Alexander Sendobry, Stefan Kohlbrecher, Uwe Klin-
National Science Foundation (SNSF), the Intel Network on                              gauf, and Oskar Von Stryk. Comprehensive simulation of quadrotor
Intelligent Systems, the European Union’s Horizon 2020 Re-                            uavs using ros and gazebo. In International conference on simulation,
                                                                                      modeling, and programming for autonomous robots, pages 400–411.
search and Innovation Programme under grant agreement No.                             Springer, 2012.
871479 (AERIAL-CORE) and the European Research Council                           [21] Gabriel Hoffmann, Haomiao Huang, Steven Waslander, and Claire
(ERC) under grant agreement No. 864042 (AGILEFLIGHT).                                 Tomlin. Quadrotor helicopter flight dynamics and control: Theory and
                                                                                      experiment. In AIAA guidance, navigation and control conference and
                              R EFERENCES                                             exhibit, page 6461, 2007.
                                                                                 [22] Haomiao Huang, Gabriel M Hoffmann, Steven L Waslander, and Claire J
 [1] Gilhyun Ryou, Ezra Tal, and Sertac Karaman. Multi-fidelity black-                Tomlin. Aerodynamics and control of autonomous quadrotor helicopters
     box optimization for time-optimal quadrotor maneuvers. In Robotics:              in aggressive maneuvering. In 2009 IEEE international conference on
     Science and Systems (RSS), 2020.                                                 robotics and automation, pages 3277–3282. IEEE, 2009.
 [2] G. Loianno, C. Brunner, G. McGrath, and V. Kumar. Estimation, control,      [23] Gabriel M Hoffmann, Haomiao Huang, Steven L Waslander, and Claire J
     and planning for aggressive flight with a small quadrotor with a single          Tomlin. Precision flight control for a multi-vehicle quadrotor helicopter
     camera and imu. IEEE Robotics and Automation Letters, 2(2):404–411,              testbed. Control engineering practice, 19(9):1023–1036, 2011.
     2017. doi: 10.1109/LRA.2016.2633290.                                        [24] Raymond W Prouty. Helicopter performance, stability, and control.
 [3] Elia Kaufmann, Antonio Loquercio, René Ranftl, Matthias Müller,                1995.
     Vladlen Koltun, and Davide Scaramuzza. Deep drone acrobatics. RSS:          [25] Matko Orsag and Stjepan Bogdan. Influence of forward and descent
     Robotics, Science, and Systems, 2020.                                            flight on quadrotor dynamics. Recent Advances in Aircraft Technology,
 [4] Philipp Foehn, Angel Romero, and Davide Scaramuzza. Time-optimal                 pages 141–156, 2012.
     planning for quadrotor waypoint flight. Science Robotics, 2021.             [26] Pierre-Jean Bristeau, Philippe Martin, Erwan Salaün, and Nicolas Petit.
 [5] Hyungpil Moon, Jose Martinez-Carranza, Titus Cieslewski, Matthias                The role of propeller aerodynamics in the model of a quadrotor uav. In
     Faessler, Davide Falanga, Alessandro Simovic, Davide Scaramuzza,                 2009 European control conference (ECC), pages 683–688. IEEE, 2009.
     Shuo Li, Michael Ozo, Christophe De Wagter, et al. Challenges and           [27] Derya Kaya and Ali T Kutay. Aerodynamic modeling and parameter
     implemented technologies used in autonomous drone racing. Intelligent            estimation of a quadrotor helicopter. In AIAA Atmospheric Flight
     Service Robotics, 12(2):137–148, 2019.                                           Mechanics Conference, page 2558, 2014.
 [6] R. Madaan, N. Gyde, S. Vemprala, M. Brown, K. Nagami, T. Taubner,           [28] Yi-Rui Tang and Yangmin Li. Dynamic modeling for high-performance
     E. Cristofalo, D. Scaramuzza, M. Schwager, and A. Kapoor. Airsim                 controller design of a uav quadrotor. In 2015 IEEE International
     drone racing lab. In PLMR Post Proceedings of the NeurIPS 2019                   Conference on Information and Automation, pages 3112–3117. IEEE,
     Competition Track, 2020.                                                         2015.
 [7] Philipp Foehn, Dario Brescianini, Elia Kaufmann, Titus Cieslewski,          [29] Caitlin Powers, Daniel Mellinger, Aleksandr Kushleyev, Bruce Koth-
     Mathias Gehrig, Manasi Muglikar, and Davide Scaramuzza. Alphapilot:              mann, and Vijay Kumar. Influence of aerodynamics and proximity
     Autonomous drone racing. RSS: Robotics, Science, and Systems, 2020.              effects in quadrotor flight. In Experimental robotics, pages 289–302.
 [8] Ali Punjani and Pieter Abbeel. Deep learning helicopter dynamics                 Springer, 2013.
     models. In 2015 IEEE International Conference on Robotics and               [30] Waqas Khan and Meyer Nahon. Toward an accurate physics-based uav
     Automation (ICRA), pages 3223–3230. IEEE, 2015.                                  thruster model. IEEE/ASME Transactions on Mechatronics, 18(4):1269–
 [9] Somil Bansal, Anayo K Akametalu, Frank J Jiang, Forrest Laine, and               1279, 2013.
     Claire J Tomlin. Learning quadrotor dynamics using neural network for       [31] Rajan Gill and Raffaello D’Andrea. Propeller thrust and drag in
     flight control. In 2016 IEEE 55th Conference on Decision and Control             forward flight. In 2017 IEEE Conference on Control Technology and
     (CDC), pages 4653–4660. IEEE, 2016.                                              Applications (CCTA), pages 73–79. IEEE, 2017.
[10] Fadri Furrer, Michael Burri, Markus Achtelik, and Roland Siegwart. Ro-      [32] Rajan Gill and Raffaello D’Andrea. Computationally efficient force and
     tors—a modular gazebo mav simulator framework. In Robot Operating                moment models for propellers in uav forward flight applications. Drones,
     System (ROS), pages 595–625. Springer, 2016.                                     3(4):77, 2019.
[11] Shital Shah, Debadeepta Dey, Chris Lovett, and Ashish Kapoor. Airsim:       [33] Carl Russell, Jaewoo Jung, Gina Willink, and Brett Glasner. Wind tunnel
     High-fidelity visual and physical simulation for autonomous vehicles. In         and hover performance test results for multicopter uas vehicles. In AHS
     Field and service robotics, pages 621–635. Springer, 2018.                       72nd annual forum, pages 16–19, 2016.
[12] Matthias Faessler, Antonio Franchi, and Davide Scaramuzza. Differ-          [34] Fabrizio Schiano, Javier Alonso-Mora, Konrad Rudin, Paul Beardsley,
     ential flatness of quadrotor dynamics subject to rotor drag for accurate         Roland Y Siegwart, and Bruno Sicilianok. Towards estimation and cor-
     tracking of high-speed trajectories. IEEE Robot. Autom. Lett., 2017.             rection of wind effects on a quadrotor uav. In IMAV 2014: International
[13] Sihao Sun, Coen C de Visser, and Qiping Chu. Quadrotor gray-box                  Micro Air Vehicle Conference and Competition 2014, pages 134–141,
     model identification from high-speed flight data. Journal of Aircraft, 56        2014.
     (2):645–661, 2019.                                                          [35] Engin Baris, Colin P Britcher, and George Altamirano. Wind tunnel test-
[14] Patricia Ventura Diaz and Steven Yoon. High-fidelity computational               ing of static and dynamic aerodynamic characteristics of a quadcopter.
     aerodynamics of multi-rotor unmanned aerial vehicles. In 2018 AIAA               In AIAA Aviation 2019 Forum, page 2973, 2019.
     Aerospace Sciences Meeting, page 1266, 2018.                                [36] Guillem Torrente, Elia Kaufmann, Philipp Föhn, and Davide Scara-
[15] Nima Mohajerin, Melissa Mozifian, and Steven Waslander. Deep                     muzza. Data-driven MPC for quadrotors. IEEE Robot. Autom. Lett.,
     learning a quadrotor dynamic model for multi-step prediction. In 2018            2021.
[37] Jinglin Luo, Longfei Zhu, and Guirong Yan. Novel quadrotor forward-         May 2019. doi: 10.1109/icra.2019.8794351. URL http://dx.doi.org/10.
     flight model based on wake interference. Aiaa Journal, 53(12):3522–         1109/ICRA.2019.8794351.
     3533, 2015.                                                            [40] Guillaume Ducard and Minh-Duc Hua. Modeling of an unmanned
[38] Nima Mohajerin and Steven Waslander. Multistep prediction of dynamic        hybrid aerial vehicle. In 2014 IEEE Conference on Control Applications
     systems with recurrent neural networks. IEEE Transactions on Neural         (CCA), pages 1011–1016. IEEE, 2014.
     Networks and Learning Systems, 2019.                                   [41] Aaron van den Oord, Sander Dieleman, Heiga Zen, Karen Simonyan,
[39] Guanya Shi, Xichen Shi, Michael O’Connell, Rose Yu, Kamyar Aziz-            Oriol Vinyals, Alex Graves, Nal Kalchbrenner, Andrew Senior, and
     zadenesheli, Animashree Anandkumar, Yisong Yue, and Soon-Jo Chung.          Koray Kavukcuoglu. Wavenet: A generative model for raw audio. arXiv
     Neural lander: Stable drone landing control using learned dynamics.         preprint arXiv:1609.03499, 2016.
     2019 International Conference on Robotics and Automation (ICRA),
