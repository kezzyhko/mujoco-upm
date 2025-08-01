// Copyright 2021 DeepMind Technologies Limited
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef MUJOCO_MUJOCO_H_
#define MUJOCO_MUJOCO_H_

// header version; should match the library version as returned by mj_version()
#define mjVERSION_HEADER 334

// needed to define size_t, fabs and log10
#include <stdlib.h>
#include <math.h>

// type definitions
#include <mujoco/mjdata.h>
#include <mujoco/mjexport.h>
#include <mujoco/mjmodel.h>
#include <mujoco/mjmacro.h>
#include <mujoco/mjplugin.h>
#include <mujoco/mjrender.h>
#include <mujoco/mjsan.h>
#include <mujoco/mjspec.h>
#include <mujoco/mjthread.h>
#include <mujoco/mjtnum.h>
#include <mujoco/mjui.h>
#include <mujoco/mjvisualize.h>

// this is a C-API
#ifdef __cplusplus
extern "C" {
#endif

// user error and memory handlers
MJAPI extern void  (*mju_user_error)(const char*);
MJAPI extern void  (*mju_user_warning)(const char*);
MJAPI extern void* (*mju_user_malloc)(size_t);
MJAPI extern void  (*mju_user_free)(void*);


// callbacks extending computation pipeline
MJAPI extern mjfGeneric  mjcb_passive;
MJAPI extern mjfGeneric  mjcb_control;
MJAPI extern mjfConFilt  mjcb_contactfilter;
MJAPI extern mjfSensor   mjcb_sensor;
MJAPI extern mjfTime     mjcb_time;
MJAPI extern mjfAct      mjcb_act_dyn;
MJAPI extern mjfAct      mjcb_act_gain;
MJAPI extern mjfAct      mjcb_act_bias;


// collision function table
MJAPI extern mjfCollision mjCOLLISIONFUNC[mjNGEOMTYPES][mjNGEOMTYPES];


// string names
MJAPI extern const char* mjDISABLESTRING[mjNDISABLE];
MJAPI extern const char* mjENABLESTRING[mjNENABLE];
MJAPI extern const char* mjTIMERSTRING[mjNTIMER];
MJAPI extern const char* mjLABELSTRING[mjNLABEL];
MJAPI extern const char* mjFRAMESTRING[mjNFRAME];
MJAPI extern const char* mjVISSTRING[mjNVISFLAG][3];
MJAPI extern const char* mjRNDSTRING[mjNRNDFLAG][3];


//---------------------------------- Virtual file system -------------------------------------------

// Initialize an empty VFS, mj_deleteVFS must be called to deallocate the VFS.
MJAPI void mj_defaultVFS(mjVFS* vfs);

// Add file to VFS, return 0: success, 2: repeated name, -1: failed to load.
MJAPI int mj_addFileVFS(mjVFS* vfs, const char* directory, const char* filename);

// Add file to VFS from buffer, return 0: success, 2: repeated name, -1: failed to load.
MJAPI int mj_addBufferVFS(mjVFS* vfs, const char* name, const void* buffer, int nbuffer);

// Delete file from VFS, return 0: success, -1: not found in VFS.
MJAPI int mj_deleteFileVFS(mjVFS* vfs, const char* filename);

// Delete all files from VFS and deallocates VFS internal memory.
MJAPI void mj_deleteVFS(mjVFS* vfs);


//---------------------------------- Parse and compile ---------------------------------------------

// Parse XML file in MJCF or URDF format, compile it, return low-level model.
// If vfs is not NULL, look up files in vfs before reading from disk.
// If error is not NULL, it must have size error_sz.
MJAPI mjModel* mj_loadXML(const char* filename, const mjVFS* vfs, char* error, int error_sz);

// Parse spec from XML file.
MJAPI mjSpec* mj_parseXML(const char* filename, const mjVFS* vfs, char* error, int error_sz);

// Parse spec from XML string.
MJAPI mjSpec* mj_parseXMLString(const char* xml, const mjVFS* vfs, char* error, int error_sz);

// Compile spec to model.
MJAPI mjModel* mj_compile(mjSpec* s, const mjVFS* vfs);

// Copy real-valued arrays from model to spec, returns 1 on success.
MJAPI int mj_copyBack(mjSpec* s, const mjModel* m);

// Recompile spec to model, preserving the state, return 0 on success.
MJAPI int mj_recompile(mjSpec* s, const mjVFS* vfs, mjModel* m, mjData* d);

// Update XML data structures with info from low-level model created with mj_loadXML, save as MJCF.
// If error is not NULL, it must have size error_sz.
MJAPI int mj_saveLastXML(const char* filename, const mjModel* m, char* error, int error_sz);

// Free last XML model if loaded. Called internally at each load.
MJAPI void mj_freeLastXML(void);

// Save spec to XML string, return 0 on success, -1 on failure.
// If length of the output buffer is too small, returns the required size.
MJAPI int mj_saveXMLString(const mjSpec* s, char* xml, int xml_sz, char* error, int error_sz);

// Save spec to XML file, return 0 on success, -1 otherwise.
MJAPI int mj_saveXML(const mjSpec* s, const char* filename, char* error, int error_sz);


//---------------------------------- Main simulation -----------------------------------------------

// Advance simulation, use control callback to obtain external force and control.
MJAPI void mj_step(const mjModel* m, mjData* d);

// Advance simulation in two steps: before external force and control is set by user.
MJAPI void mj_step1(const mjModel* m, mjData* d);

// Advance simulation in two steps: after external force and control is set by user.
MJAPI void mj_step2(const mjModel* m, mjData* d);

// Forward dynamics: same as mj_step but do not integrate in time.
MJAPI void mj_forward(const mjModel* m, mjData* d);

// Inverse dynamics: qacc must be set before calling.
MJAPI void mj_inverse(const mjModel* m, mjData* d);

// Forward dynamics with skip; skipstage is mjtStage.
MJAPI void mj_forwardSkip(const mjModel* m, mjData* d, int skipstage, int skipsensor);

// Inverse dynamics with skip; skipstage is mjtStage.
MJAPI void mj_inverseSkip(const mjModel* m, mjData* d, int skipstage, int skipsensor);


//---------------------------------- Initialization ------------------------------------------------

// Set default options for length range computation.
MJAPI void mj_defaultLROpt(mjLROpt* opt);

// Set solver parameters to default values.
MJAPI void mj_defaultSolRefImp(mjtNum* solref, mjtNum* solimp);

// Set physics options to default values.
MJAPI void mj_defaultOption(mjOption* opt);

// Set visual options to default values.
MJAPI void mj_defaultVisual(mjVisual* vis);

// Copy mjModel, allocate new if dest is NULL.
MJAPI mjModel* mj_copyModel(mjModel* dest, const mjModel* src);

// Save model to binary MJB file or memory buffer; buffer has precedence when given.
MJAPI void mj_saveModel(const mjModel* m, const char* filename, void* buffer, int buffer_sz);

// Load model from binary MJB file.
// If vfs is not NULL, look up file in vfs before reading from disk.
MJAPI mjModel* mj_loadModel(const char* filename, const mjVFS* vfs);

// Free memory allocation in model.
MJAPI void mj_deleteModel(mjModel* m);

// Return size of buffer needed to hold model.
MJAPI int mj_sizeModel(const mjModel* m);

// Allocate mjData corresponding to given model.
// If the model buffer is unallocated the initial configuration will not be set.
MJAPI mjData* mj_makeData(const mjModel* m);

// Copy mjData.
// m is only required to contain the size fields from MJMODEL_INTS.
MJAPI mjData* mj_copyData(mjData* dest, const mjModel* m, const mjData* src);

// Copy mjData, skip large arrays not required for visualization.
MJAPI mjData* mjv_copyData(mjData* dest, const mjModel* m, const mjData* src);

// Reset data to defaults.
MJAPI void mj_resetData(const mjModel* m, mjData* d);

// Reset data to defaults, fill everything else with debug_value.
MJAPI void mj_resetDataDebug(const mjModel* m, mjData* d, unsigned char debug_value);

// Reset data. If 0 <= key < nkey, set fields from specified keyframe.
MJAPI void mj_resetDataKeyframe(const mjModel* m, mjData* d, int key);

#ifndef ADDRESS_SANITIZER  // Stack management functions declared in mjsan.h if ASAN is active.

// Mark a new frame on the mjData stack.
MJAPI void mj_markStack(mjData* d);

// Free the current mjData stack frame. All pointers returned by mj_stackAlloc since the last call
// to mj_markStack must no longer be used afterwards.
MJAPI void mj_freeStack(mjData* d);

#endif  // ADDRESS_SANITIZER

// Allocate a number of bytes on mjData stack at a specific alignment.
// Call mju_error on stack overflow.
MJAPI void* mj_stackAllocByte(mjData* d, size_t bytes, size_t alignment);

// Allocate array of mjtNums on mjData stack. Call mju_error on stack overflow.
MJAPI mjtNum* mj_stackAllocNum(mjData* d, size_t size);

// Allocate array of ints on mjData stack. Call mju_error on stack overflow.
MJAPI int* mj_stackAllocInt(mjData* d, size_t size);

// Free memory allocation in mjData.
MJAPI void mj_deleteData(mjData* d);

// Reset all callbacks to NULL pointers (NULL is the default).
MJAPI void mj_resetCallbacks(void);

// Set constant fields of mjModel, corresponding to qpos0 configuration.
MJAPI void mj_setConst(mjModel* m, mjData* d);

// Set actuator_lengthrange for specified actuator; return 1 if ok, 0 if error.
MJAPI int mj_setLengthRange(mjModel* m, mjData* d, int index,
                            const mjLROpt* opt, char* error, int error_sz);

// Create empty spec.
MJAPI mjSpec* mj_makeSpec(void);

// Copy spec.
MJAPI mjSpec* mj_copySpec(const mjSpec* s);

// Free memory allocation in mjSpec.
MJAPI void mj_deleteSpec(mjSpec* s);

// Activate plugin. Returns 0 on success.
MJAPI int mjs_activatePlugin(mjSpec* s, const char* name);

// Turn deep copy on or off attach. Returns 0 on success.
MJAPI int mjs_setDeepCopy(mjSpec* s, int deepcopy);


//---------------------------------- Printing ------------------------------------------------------

// Print mjModel to text file, specifying format.
// float_format must be a valid printf-style format string for a single float value.
MJAPI void mj_printFormattedModel(const mjModel* m, const char* filename, const char* float_format);

// Print model to text file.
MJAPI void mj_printModel(const mjModel* m, const char* filename);

// Print mjData to text file, specifying format.
// float_format must be a valid printf-style format string for a single float value
MJAPI void mj_printFormattedData(const mjModel* m, const mjData* d, const char* filename,
                                 const char* float_format);

// Print data to text file.
MJAPI void mj_printData(const mjModel* m, const mjData* d, const char* filename);

// Print matrix to screen.
MJAPI void mju_printMat(const mjtNum* mat, int nr, int nc);

// Print sparse matrix to screen.
MJAPI void mju_printMatSparse(const mjtNum* mat, int nr,
                              const int* rownnz, const int* rowadr, const int* colind);

// Print internal XML schema as plain text or HTML, with style-padding or &nbsp;.
MJAPI int mj_printSchema(const char* filename, char* buffer, int buffer_sz,
                         int flg_html, int flg_pad);


//---------------------------------- Components ----------------------------------------------------

// Run position-dependent computations.
MJAPI void mj_fwdPosition(const mjModel* m, mjData* d);

// Run velocity-dependent computations.
MJAPI void mj_fwdVelocity(const mjModel* m, mjData* d);

// Compute actuator force qfrc_actuator.
MJAPI void mj_fwdActuation(const mjModel* m, mjData* d);

// Add up all non-constraint forces, compute qacc_smooth.
MJAPI void mj_fwdAcceleration(const mjModel* m, mjData* d);

// Run selected constraint solver.
MJAPI void mj_fwdConstraint(const mjModel* m, mjData* d);

// Euler integrator, semi-implicit in velocity.
MJAPI void mj_Euler(const mjModel* m, mjData* d);

// Runge-Kutta explicit order-N integrator.
MJAPI void mj_RungeKutta(const mjModel* m, mjData* d, int N);

// Implicit-in-velocity integrators.
MJAPI void mj_implicit(const mjModel* m, mjData* d);

// Run position-dependent computations in inverse dynamics.
MJAPI void mj_invPosition(const mjModel* m, mjData* d);

// Run velocity-dependent computations in inverse dynamics.
MJAPI void mj_invVelocity(const mjModel* m, mjData* d);

// Apply the analytical formula for inverse constraint dynamics.
MJAPI void mj_invConstraint(const mjModel* m, mjData* d);

// Compare forward and inverse dynamics, save results in fwdinv.
MJAPI void mj_compareFwdInv(const mjModel* m, mjData* d);


//---------------------------------- Sub components ------------------------------------------------

// Evaluate position-dependent sensors.
MJAPI void mj_sensorPos(const mjModel* m, mjData* d);

// Evaluate velocity-dependent sensors.
MJAPI void mj_sensorVel(const mjModel* m, mjData* d);

// Evaluate acceleration and force-dependent sensors.
MJAPI void mj_sensorAcc(const mjModel* m, mjData* d);

// Evaluate position-dependent energy (potential).
MJAPI void mj_energyPos(const mjModel* m, mjData* d);

// Evaluate velocity-dependent energy (kinetic).
MJAPI void mj_energyVel(const mjModel* m, mjData* d);

// Check qpos, reset if any element is too big or nan.
MJAPI void mj_checkPos(const mjModel* m, mjData* d);

// Check qvel, reset if any element is too big or nan.
MJAPI void mj_checkVel(const mjModel* m, mjData* d);

// Check qacc, reset if any element is too big or nan.
MJAPI void mj_checkAcc(const mjModel* m, mjData* d);

// Run forward kinematics.
MJAPI void mj_kinematics(const mjModel* m, mjData* d);

// Map inertias and motion dofs to global frame centered at CoM.
MJAPI void mj_comPos(const mjModel* m, mjData* d);

// Compute camera and light positions and orientations.
MJAPI void mj_camlight(const mjModel* m, mjData* d);

// Compute flex-related quantities.
MJAPI void mj_flex(const mjModel* m, mjData* d);

// Compute tendon lengths, velocities and moment arms.
MJAPI void mj_tendon(const mjModel* m, mjData* d);

// Compute actuator transmission lengths and moments.
MJAPI void mj_transmission(const mjModel* m, mjData* d);

// Run composite rigid body inertia algorithm (CRB).
MJAPI void mj_crb(const mjModel* m, mjData* d);

// Make inertia matrix.
MJAPI void mj_makeM(const mjModel* m, mjData* d);

// Compute sparse L'*D*L factorizaton of inertia matrix.
MJAPI void mj_factorM(const mjModel* m, mjData* d);

// Solve linear system M * x = y using factorization:  x = inv(L'*D*L)*y
MJAPI void mj_solveM(const mjModel* m, mjData* d, mjtNum* x, const mjtNum* y, int n);

// Half of linear solve:  x = sqrt(inv(D))*inv(L')*y
MJAPI void mj_solveM2(const mjModel* m, mjData* d, mjtNum* x, const mjtNum* y,
                      const mjtNum* sqrtInvD, int n);

// Compute cvel, cdof_dot.
MJAPI void mj_comVel(const mjModel* m, mjData* d);

// Compute qfrc_passive from spring-dampers, gravity compensation and fluid forces.
MJAPI void mj_passive(const mjModel* m, mjData* d);

// Sub-tree linear velocity and angular momentum: compute subtree_linvel, subtree_angmom.
MJAPI void mj_subtreeVel(const mjModel* m, mjData* d);

// RNE: compute M(qpos)*qacc + C(qpos,qvel); flg_acc=0 removes inertial term.
MJAPI void mj_rne(const mjModel* m, mjData* d, int flg_acc, mjtNum* result);

// RNE with complete data: compute cacc, cfrc_ext, cfrc_int.
MJAPI void mj_rnePostConstraint(const mjModel* m, mjData* d);

// Run collision detection.
MJAPI void mj_collision(const mjModel* m, mjData* d);

// Construct constraints.
MJAPI void mj_makeConstraint(const mjModel* m, mjData* d);

// Find constraint islands.
MJAPI void mj_island(const mjModel* m, mjData* d);

// Compute inverse constraint inertia efc_AR.
MJAPI void mj_projectConstraint(const mjModel* m, mjData* d);

// Compute efc_vel, efc_aref.
MJAPI void mj_referenceConstraint(const mjModel* m, mjData* d);

// Compute efc_state, efc_force, qfrc_constraint, and (optionally) cone Hessians.
// If cost is not NULL, set *cost = s(jar) where jar = Jac*qacc-aref.
MJAPI void mj_constraintUpdate(const mjModel* m, mjData* d, const mjtNum* jar,
                               mjtNum cost[1], int flg_coneHessian);


//---------------------------------- Support -------------------------------------------------------

// Return size of state specification.
MJAPI int mj_stateSize(const mjModel* m, unsigned int spec);

// Get state.
MJAPI void mj_getState(const mjModel* m, const mjData* d, mjtNum* state, unsigned int spec);

// Set state.
MJAPI void mj_setState(const mjModel* m, mjData* d, const mjtNum* state, unsigned int spec);

// Copy current state to the k-th model keyframe.
MJAPI void mj_setKeyframe(mjModel* m, const mjData* d, int k);

// Add contact to d->contact list; return 0 if success; 1 if buffer full.
MJAPI int mj_addContact(const mjModel* m, mjData* d, const mjContact* con);

// Determine type of friction cone.
MJAPI int mj_isPyramidal(const mjModel* m);

// Determine type of constraint Jacobian.
MJAPI int mj_isSparse(const mjModel* m);

// Determine type of solver (PGS is dual, CG and Newton are primal).
MJAPI int mj_isDual(const mjModel* m);

// Multiply dense or sparse constraint Jacobian by vector.
MJAPI void mj_mulJacVec(const mjModel* m, const mjData* d, mjtNum* res, const mjtNum* vec);

// Multiply dense or sparse constraint Jacobian transpose by vector.
MJAPI void mj_mulJacTVec(const mjModel* m, const mjData* d, mjtNum* res, const mjtNum* vec);

// Compute 3/6-by-nv end-effector Jacobian of global point attached to given body.
MJAPI void mj_jac(const mjModel* m, const mjData* d, mjtNum* jacp, mjtNum* jacr,
                  const mjtNum point[3], int body);

// Compute body frame end-effector Jacobian.
MJAPI void mj_jacBody(const mjModel* m, const mjData* d, mjtNum* jacp, mjtNum* jacr, int body);

// Compute body center-of-mass end-effector Jacobian.
MJAPI void mj_jacBodyCom(const mjModel* m, const mjData* d, mjtNum* jacp, mjtNum* jacr, int body);

// Compute subtree center-of-mass end-effector Jacobian.
MJAPI void mj_jacSubtreeCom(const mjModel* m, mjData* d, mjtNum* jacp, int body);

// Compute geom end-effector Jacobian.
MJAPI void mj_jacGeom(const mjModel* m, const mjData* d, mjtNum* jacp, mjtNum* jacr, int geom);

// Compute site end-effector Jacobian.
MJAPI void mj_jacSite(const mjModel* m, const mjData* d, mjtNum* jacp, mjtNum* jacr, int site);

// Compute translation end-effector Jacobian of point, and rotation Jacobian of axis.
MJAPI void mj_jacPointAxis(const mjModel* m, mjData* d, mjtNum* jacPoint, mjtNum* jacAxis,
                           const mjtNum point[3], const mjtNum axis[3], int body);

// Compute 3/6-by-nv Jacobian time derivative of global point attached to given body.
MJAPI void mj_jacDot(const mjModel* m, const mjData* d, mjtNum* jacp, mjtNum* jacr,
                     const mjtNum point[3], int body);

// Compute subtree angular momentum matrix.
MJAPI void mj_angmomMat(const mjModel* m, mjData* d, mjtNum* mat, int body);

// Get id of object with the specified mjtObj type and name, returns -1 if id not found.
MJAPI int mj_name2id(const mjModel* m, int type, const char* name);

// Get name of object with the specified mjtObj type and id, returns NULL if name not found.
MJAPI const char* mj_id2name(const mjModel* m, int type, int id);

// Convert sparse inertia matrix M into full (i.e. dense) matrix.
MJAPI void mj_fullM(const mjModel* m, mjtNum* dst, const mjtNum* M);

// Multiply vector by inertia matrix.
MJAPI void mj_mulM(const mjModel* m, const mjData* d, mjtNum* res, const mjtNum* vec);

// Multiply vector by (inertia matrix)^(1/2).
MJAPI void mj_mulM2(const mjModel* m, const mjData* d, mjtNum* res, const mjtNum* vec);

// Add inertia matrix to destination matrix.
// Destination can be sparse or dense when all int* are NULL.
MJAPI void mj_addM(const mjModel* m, mjData* d, mjtNum* dst, int* rownnz, int* rowadr, int* colind);

// Apply Cartesian force and torque (outside xfrc_applied mechanism).
MJAPI void mj_applyFT(const mjModel* m, mjData* d, const mjtNum force[3], const mjtNum torque[3],
                      const mjtNum point[3], int body, mjtNum* qfrc_target);

// Compute object 6D velocity (rot:lin) in object-centered frame, world/local orientation.
MJAPI void mj_objectVelocity(const mjModel* m, const mjData* d,
                             int objtype, int objid, mjtNum res[6], int flg_local);

// Compute object 6D acceleration (rot:lin) in object-centered frame, world/local orientation.
MJAPI void mj_objectAcceleration(const mjModel* m, const mjData* d,
                                 int objtype, int objid, mjtNum res[6], int flg_local);

// Returns smallest signed distance between two geoms and optionally segment from geom1 to geom2.
MJAPI mjtNum mj_geomDistance(const mjModel* m, const mjData* d, int geom1, int geom2,
                             mjtNum distmax, mjtNum fromto[6]);

// Extract 6D force:torque given contact id, in the contact frame.
MJAPI void mj_contactForce(const mjModel* m, const mjData* d, int id, mjtNum result[6]);

// Compute velocity by finite-differencing two positions.
MJAPI void mj_differentiatePos(const mjModel* m, mjtNum* qvel, mjtNum dt,
                               const mjtNum* qpos1, const mjtNum* qpos2);

// Integrate position with given velocity.
MJAPI void mj_integratePos(const mjModel* m, mjtNum* qpos, const mjtNum* qvel, mjtNum dt);

// Normalize all quaternions in qpos-type vector.
MJAPI void mj_normalizeQuat(const mjModel* m, mjtNum* qpos);

// Map from body local to global Cartesian coordinates, sameframe takes values from mjtSameFrame.
MJAPI void mj_local2Global(mjData* d, mjtNum xpos[3], mjtNum xmat[9], const mjtNum pos[3],
                           const mjtNum quat[4], int body, mjtByte sameframe);

// Sum all body masses.
MJAPI mjtNum mj_getTotalmass(const mjModel* m);

// Scale body masses and inertias to achieve specified total mass.
MJAPI void mj_setTotalmass(mjModel* m, mjtNum newmass);

// Return a config attribute value of a plugin instance;
// NULL: invalid plugin instance ID or attribute name
MJAPI const char* mj_getPluginConfig(const mjModel* m, int plugin_id, const char* attrib);

// Load a dynamic library. The dynamic library is assumed to register one or more plugins.
MJAPI void mj_loadPluginLibrary(const char* path);

// Scan a directory and load all dynamic libraries. Dynamic libraries in the specified directory
// are assumed to register one or more plugins. Optionally, if a callback is specified, it is called
// for each dynamic library encountered that registers plugins.
MJAPI void mj_loadAllPluginLibraries(const char* directory, mjfPluginLibraryLoadCallback callback);

// Return version number: 1.0.2 is encoded as 102.
MJAPI int mj_version(void);

// Return the current version of MuJoCo as a null-terminated string.
MJAPI const char* mj_versionString(void);


//---------------------------------- Ray casting ---------------------------------------------------

// Intersect multiple rays emanating from a single point.
// Similar semantics to mj_ray, but vec is an array of (nray x 3) directions.
MJAPI void mj_multiRay(const mjModel* m, mjData* d, const mjtNum pnt[3], const mjtNum* vec,
                       const mjtByte* geomgroup, mjtByte flg_static, int bodyexclude,
                       int* geomid, mjtNum* dist, int nray, mjtNum cutoff);

// Intersect ray (pnt+x*vec, x>=0) with visible geoms, except geoms in bodyexclude.
// Return distance (x) to nearest surface, or -1 if no intersection and output geomid.
// geomgroup, flg_static are as in mjvOption; geomgroup==NULL skips group exclusion.
MJAPI mjtNum mj_ray(const mjModel* m, const mjData* d, const mjtNum pnt[3], const mjtNum vec[3],
                    const mjtByte* geomgroup, mjtByte flg_static, int bodyexclude,
                    int geomid[1]);

// Intersect ray with hfield, return nearest distance or -1 if no intersection.
MJAPI mjtNum mj_rayHfield(const mjModel* m, const mjData* d, int geomid,
                          const mjtNum pnt[3], const mjtNum vec[3]);

// Intersect ray with mesh, return nearest distance or -1 if no intersection.
MJAPI mjtNum mj_rayMesh(const mjModel* m, const mjData* d, int geomid,
                        const mjtNum pnt[3], const mjtNum vec[3]);

// Intersect ray with pure geom, return nearest distance or -1 if no intersection.
MJAPI mjtNum mju_rayGeom(const mjtNum pos[3], const mjtNum mat[9], const mjtNum size[3],
                         const mjtNum pnt[3], const mjtNum vec[3], int geomtype);

// Intersect ray with flex, return nearest distance or -1 if no intersection,
// and also output nearest vertex id.
MJAPI mjtNum mju_rayFlex(const mjModel* m, const mjData* d, int flex_layer, mjtByte flg_vert,
                         mjtByte flg_edge, mjtByte flg_face, mjtByte flg_skin, int flexid,
                         const mjtNum* pnt, const mjtNum* vec, int vertid[1]);

// Intersect ray with skin, return nearest distance or -1 if no intersection,
// and also output nearest vertex id.
MJAPI mjtNum mju_raySkin(int nface, int nvert, const int* face, const float* vert,
                         const mjtNum pnt[3], const mjtNum vec[3], int vertid[1]);


//---------------------------------- Interaction ---------------------------------------------------

// Set default camera.
MJAPI void mjv_defaultCamera(mjvCamera* cam);

// Set default free camera.
MJAPI void mjv_defaultFreeCamera(const mjModel* m, mjvCamera* cam);

// Set default perturbation.
MJAPI void mjv_defaultPerturb(mjvPerturb* pert);

// Transform pose from room to model space.
MJAPI void mjv_room2model(mjtNum modelpos[3], mjtNum modelquat[4], const mjtNum roompos[3],
                          const mjtNum roomquat[4], const mjvScene* scn);

// Transform pose from model to room space.
MJAPI void mjv_model2room(mjtNum roompos[3], mjtNum roomquat[4], const mjtNum modelpos[3],
                          const mjtNum modelquat[4], const mjvScene* scn);

// Get camera info in model space; average left and right OpenGL cameras.
MJAPI void mjv_cameraInModel(mjtNum headpos[3], mjtNum forward[3], mjtNum up[3],
                             const mjvScene* scn);

// Get camera info in room space; average left and right OpenGL cameras.
MJAPI void mjv_cameraInRoom(mjtNum headpos[3], mjtNum forward[3], mjtNum up[3],
                            const mjvScene* scn);

// Get frustum height at unit distance from camera; average left and right OpenGL cameras.
MJAPI mjtNum mjv_frustumHeight(const mjvScene* scn);

// Rotate 3D vec in horizontal plane by angle between (0,1) and (forward_x,forward_y).
MJAPI void mjv_alignToCamera(mjtNum res[3], const mjtNum vec[3], const mjtNum forward[3]);

// Move camera with mouse; action is mjtMouse.
MJAPI void mjv_moveCamera(const mjModel* m, int action, mjtNum reldx, mjtNum reldy,
                          const mjvScene* scn, mjvCamera* cam);

// Move perturb object with mouse; action is mjtMouse.
MJAPI void mjv_movePerturb(const mjModel* m, const mjData* d, int action, mjtNum reldx,
                           mjtNum reldy, const mjvScene* scn, mjvPerturb* pert);

// Move model with mouse; action is mjtMouse.
MJAPI void mjv_moveModel(const mjModel* m, int action, mjtNum reldx, mjtNum reldy,
                         const mjtNum roomup[3], mjvScene* scn);

// Copy perturb pos,quat from selected body; set scale for perturbation.
MJAPI void mjv_initPerturb(const mjModel* m, mjData* d, const mjvScene* scn, mjvPerturb* pert);

// Set perturb pos,quat in d->mocap when selected body is mocap, and in d->qpos otherwise.
// Write d->qpos only if flg_paused and subtree root for selected body has free joint.
MJAPI void mjv_applyPerturbPose(const mjModel* m, mjData* d, const mjvPerturb* pert,
                                int flg_paused);

// Set perturb force,torque in d->xfrc_applied, if selected body is dynamic.
MJAPI void mjv_applyPerturbForce(const mjModel* m, mjData* d, const mjvPerturb* pert);

// Return the average of two OpenGL cameras.
MJAPI mjvGLCamera mjv_averageCamera(const mjvGLCamera* cam1, const mjvGLCamera* cam2);

// Select geom, flex or skin with mouse, return bodyid; -1: none selected.
MJAPI int mjv_select(const mjModel* m, const mjData* d, const mjvOption* vopt,
                     mjtNum aspectratio, mjtNum relx, mjtNum rely,
                     const mjvScene* scn, mjtNum selpnt[3],
                     int geomid[1], int flexid[1], int skinid[1]);


//---------------------------------- Visualization -------------------------------------------------

// Set default visualization options.
MJAPI void mjv_defaultOption(mjvOption* opt);

// Set default figure.
MJAPI void mjv_defaultFigure(mjvFigure* fig);

// Initialize given geom fields when not NULL, set the rest to their default values.
MJAPI void mjv_initGeom(mjvGeom* geom, int type, const mjtNum size[3],
                        const mjtNum pos[3], const mjtNum mat[9], const float rgba[4]);

// Set (type, size, pos, mat) for connector-type geom between given points.
// Assume that mjv_initGeom was already called to set all other properties.
// Width of mjGEOM_LINE is denominated in pixels.
MJAPI void mjv_connector(mjvGeom* geom, int type, mjtNum width,
                         const mjtNum from[3], const mjtNum to[3]);

// Set default abstract scene.
MJAPI void mjv_defaultScene(mjvScene* scn);

// Allocate resources in abstract scene.
MJAPI void mjv_makeScene(const mjModel* m, mjvScene* scn, int maxgeom);

// Free abstract scene.
MJAPI void mjv_freeScene(mjvScene* scn);

// Update entire scene given model state.
MJAPI void mjv_updateScene(const mjModel* m, mjData* d, const mjvOption* opt,
                           const mjvPerturb* pert, mjvCamera* cam, int catmask, mjvScene* scn);

// Copy mjModel, skip large arrays not required for abstract visualization.
MJAPI void mjv_copyModel(mjModel* dest, const mjModel* src);

// Add geoms from selected categories.
MJAPI void mjv_addGeoms(const mjModel* m, mjData* d, const mjvOption* opt,
                        const mjvPerturb* pert, int catmask, mjvScene* scn);

// Make list of lights.
MJAPI void mjv_makeLights(const mjModel* m, const mjData* d, mjvScene* scn);

// Update camera.
MJAPI void mjv_updateCamera(const mjModel* m, const mjData* d, mjvCamera* cam, mjvScene* scn);

// Update skins.
MJAPI void mjv_updateSkin(const mjModel* m, const mjData* d, mjvScene* scn);


//---------------------------------- OpenGL rendering ----------------------------------------------

// Set default mjrContext.
MJAPI void mjr_defaultContext(mjrContext* con);

// Allocate resources in custom OpenGL context; fontscale is mjtFontScale.
MJAPI void mjr_makeContext(const mjModel* m, mjrContext* con, int fontscale);

// Change font of existing context.
MJAPI void mjr_changeFont(int fontscale, mjrContext* con);

// Add Aux buffer with given index to context; free previous Aux buffer.
MJAPI void mjr_addAux(int index, int width, int height, int samples, mjrContext* con);

// Free resources in custom OpenGL context, set to default.
MJAPI void mjr_freeContext(mjrContext* con);

// Resize offscreen buffers.
MJAPI void mjr_resizeOffscreen(int width, int height, mjrContext* con);

// Upload texture to GPU, overwriting previous upload if any.
MJAPI void mjr_uploadTexture(const mjModel* m, const mjrContext* con, int texid);

// Upload mesh to GPU, overwriting previous upload if any.
MJAPI void mjr_uploadMesh(const mjModel* m, const mjrContext* con, int meshid);

// Upload height field to GPU, overwriting previous upload if any.
MJAPI void mjr_uploadHField(const mjModel* m, const mjrContext* con, int hfieldid);

// Make con->currentBuffer current again.
MJAPI void mjr_restoreBuffer(const mjrContext* con);

// Set OpenGL framebuffer for rendering: mjFB_WINDOW or mjFB_OFFSCREEN.
// If only one buffer is available, set that buffer and ignore framebuffer argument.
MJAPI void mjr_setBuffer(int framebuffer, mjrContext* con);

// Read pixels from current OpenGL framebuffer to client buffer.
// Viewport is in OpenGL framebuffer; client buffer starts at (0,0).
MJAPI void mjr_readPixels(unsigned char* rgb, float* depth,
                          mjrRect viewport, const mjrContext* con);

// Draw pixels from client buffer to current OpenGL framebuffer.
// Viewport is in OpenGL framebuffer; client buffer starts at (0,0).
MJAPI void mjr_drawPixels(const unsigned char* rgb, const float* depth,
                          mjrRect viewport, const mjrContext* con);

// Blit from src viewpoint in current framebuffer to dst viewport in other framebuffer.
// If src, dst have different size and flg_depth==0, color is interpolated with GL_LINEAR.
MJAPI void mjr_blitBuffer(mjrRect src, mjrRect dst,
                          int flg_color, int flg_depth, const mjrContext* con);

// Set Aux buffer for custom OpenGL rendering (call restoreBuffer when done).
MJAPI void mjr_setAux(int index, const mjrContext* con);

// Blit from Aux buffer to con->currentBuffer.
MJAPI void mjr_blitAux(int index, mjrRect src, int left, int bottom, const mjrContext* con);

// Draw text at (x,y) in relative coordinates; font is mjtFont.
MJAPI void mjr_text(int font, const char* txt, const mjrContext* con,
                    float x, float y, float r, float g, float b);

// Draw text overlay; font is mjtFont; gridpos is mjtGridPos.
MJAPI void mjr_overlay(int font, int gridpos, mjrRect viewport,
                       const char* overlay, const char* overlay2, const mjrContext* con);

// Get maximum viewport for active buffer.
MJAPI mjrRect mjr_maxViewport(const mjrContext* con);

// Draw rectangle.
MJAPI void mjr_rectangle(mjrRect viewport, float r, float g, float b, float a);

// Draw rectangle with centered text.
MJAPI void mjr_label(mjrRect viewport, int font, const char* txt,
                     float r, float g, float b, float a, float rt, float gt, float bt,
                     const mjrContext* con);

// Draw 2D figure.
MJAPI void mjr_figure(mjrRect viewport, mjvFigure* fig, const mjrContext* con);

// Render 3D scene.
MJAPI void mjr_render(mjrRect viewport, mjvScene* scn, const mjrContext* con);

// Call glFinish.
MJAPI void mjr_finish(void);

// Call glGetError and return result.
MJAPI int mjr_getError(void);

// Find first rectangle containing mouse, -1: not found.
MJAPI int mjr_findRect(int x, int y, int nrect, const mjrRect* rect);


//---------------------------------- UI framework --------------------------------------------------

// Get builtin UI theme spacing (ind: 0-1).
MJAPI mjuiThemeSpacing mjui_themeSpacing(int ind);

// Get builtin UI theme color (ind: 0-3).
MJAPI mjuiThemeColor mjui_themeColor(int ind);

// Add definitions to UI.
MJAPI void mjui_add(mjUI* ui, const mjuiDef* def);

// Add definitions to UI section.
MJAPI void mjui_addToSection(mjUI* ui, int sect, const mjuiDef* def);

// Compute UI sizes.
MJAPI void mjui_resize(mjUI* ui, const mjrContext* con);

// Update specific section/item; -1: update all.
MJAPI void mjui_update(int section, int item, const mjUI* ui,
                       const mjuiState* state, const mjrContext* con);

// Handle UI event, return pointer to changed item, NULL if no change.
MJAPI mjuiItem* mjui_event(mjUI* ui, mjuiState* state, const mjrContext* con);

// Copy UI image to current buffer.
MJAPI void mjui_render(mjUI* ui, const mjuiState* state, const mjrContext* con);


//---------------------------------- Error and memory ----------------------------------------------

// Main error function; does not return to caller.
MJAPI void mju_error(const char* msg, ...) mjPRINTFLIKE(1, 2);

// Deprecated: use mju_error.
MJAPI void mju_error_i(const char* msg, int i);

// Deprecated: use mju_error.
MJAPI void mju_error_s(const char* msg, const char* text);

// Main warning function; returns to caller.
MJAPI void mju_warning(const char* msg, ...) mjPRINTFLIKE(1, 2);

// Deprecated: use mju_warning.
MJAPI void mju_warning_i(const char* msg, int i);

// Deprecated: use mju_warning.
MJAPI void mju_warning_s(const char* msg, const char* text);

// Clear user error and memory handlers.
MJAPI void mju_clearHandlers(void);

// Allocate memory; byte-align on 64; pad size to multiple of 64.
MJAPI void* mju_malloc(size_t size);

// Free memory, using free() by default.
MJAPI void mju_free(void* ptr);

// High-level warning function: count warnings in mjData, print only the first.
MJAPI void mj_warning(mjData* d, int warning, int info);

// Write [datetime, type: message] to MUJOCO_LOG.TXT.
MJAPI void mju_writeLog(const char* type, const char* msg);

// Get compiler error message from spec.
MJAPI const char* mjs_getError(mjSpec* s);

// Return 1 if compiler error is a warning.
MJAPI int mjs_isWarning(mjSpec* s);


//---------------------------------- Standard math -------------------------------------------------

#if !defined(mjUSESINGLE)
  #define mju_sqrt    sqrt
  #define mju_exp     exp
  #define mju_sin     sin
  #define mju_cos     cos
  #define mju_tan     tan
  #define mju_asin    asin
  #define mju_acos    acos
  #define mju_atan2   atan2
  #define mju_tanh    tanh
  #define mju_pow     pow
  #define mju_abs     fabs
  #define mju_log     log
  #define mju_log10   log10
  #define mju_floor   floor
  #define mju_ceil    ceil

#else
  #define mju_sqrt    sqrtf
  #define mju_exp     expf
  #define mju_sin     sinf
  #define mju_cos     cosf
  #define mju_tan     tanf
  #define mju_asin    asinf
  #define mju_acos    acosf
  #define mju_atan2   atan2f
  #define mju_tanh    tanhf
  #define mju_pow     powf
  #define mju_abs     fabsf
  #define mju_log     logf
  #define mju_log10   log10f
  #define mju_floor   floorf
  #define mju_ceil    ceilf
#endif


//---------------------------------- Vector math ---------------------------------------------------

// Set res = 0.
MJAPI void mju_zero3(mjtNum res[3]);

// Set res = vec.
MJAPI void mju_copy3(mjtNum res[3], const mjtNum data[3]);

// Set res = vec*scl.
MJAPI void mju_scl3(mjtNum res[3], const mjtNum vec[3], mjtNum scl);

// Set res = vec1 + vec2.
MJAPI void mju_add3(mjtNum res[3], const mjtNum vec1[3], const mjtNum vec2[3]);

// Set res = vec1 - vec2.
MJAPI void mju_sub3(mjtNum res[3], const mjtNum vec1[3], const mjtNum vec2[3]);

// Set res = res + vec.
MJAPI void mju_addTo3(mjtNum res[3], const mjtNum vec[3]);

// Set res = res - vec.
MJAPI void mju_subFrom3(mjtNum res[3], const mjtNum vec[3]);

// Set res = res + vec*scl.
MJAPI void mju_addToScl3(mjtNum res[3], const mjtNum vec[3], mjtNum scl);

// Set res = vec1 + vec2*scl.
MJAPI void mju_addScl3(mjtNum res[3], const mjtNum vec1[3], const mjtNum vec2[3], mjtNum scl);

// Normalize vector, return length before normalization.
MJAPI mjtNum mju_normalize3(mjtNum vec[3]);

// Return vector length (without normalizing the vector).
MJAPI mjtNum mju_norm3(const mjtNum vec[3]);

// Return dot-product of vec1 and vec2.
MJAPI mjtNum mju_dot3(const mjtNum vec1[3], const mjtNum vec2[3]);

// Return Cartesian distance between 3D vectors pos1 and pos2.
MJAPI mjtNum mju_dist3(const mjtNum pos1[3], const mjtNum pos2[3]);

// Multiply 3-by-3 matrix by vector: res = mat * vec.
MJAPI void mju_mulMatVec3(mjtNum res[3], const mjtNum mat[9], const mjtNum vec[3]);

// Multiply transposed 3-by-3 matrix by vector: res = mat' * vec.
MJAPI void mju_mulMatTVec3(mjtNum res[3], const mjtNum mat[9], const mjtNum vec[3]);

// Compute cross-product: res = cross(a, b).
MJAPI void mju_cross(mjtNum res[3], const mjtNum a[3], const mjtNum b[3]);

// Set res = 0.
MJAPI void mju_zero4(mjtNum res[4]);

// Set res = (1,0,0,0).
MJAPI void mju_unit4(mjtNum res[4]);

// Set res = vec.
MJAPI void mju_copy4(mjtNum res[4], const mjtNum data[4]);

// Normalize vector, return length before normalization.
MJAPI mjtNum mju_normalize4(mjtNum vec[4]);

// Set res = 0.
MJAPI void mju_zero(mjtNum* res, int n);

// Set res = val.
MJAPI void mju_fill(mjtNum* res, mjtNum val, int n);

// Set res = vec.
MJAPI void mju_copy(mjtNum* res, const mjtNum* vec, int n);

// Return sum(vec).
MJAPI mjtNum mju_sum(const mjtNum* vec, int n);

// Return L1 norm: sum(abs(vec)).
MJAPI mjtNum mju_L1(const mjtNum* vec, int n);

// Set res = vec*scl.
MJAPI void mju_scl(mjtNum* res, const mjtNum* vec, mjtNum scl, int n);

// Set res = vec1 + vec2.
MJAPI void mju_add(mjtNum* res, const mjtNum* vec1, const mjtNum* vec2, int n);

// Set res = vec1 - vec2.
MJAPI void mju_sub(mjtNum* res, const mjtNum* vec1, const mjtNum* vec2, int n);

// Set res = res + vec.
MJAPI void mju_addTo(mjtNum* res, const mjtNum* vec, int n);

// Set res = res - vec.
MJAPI void mju_subFrom(mjtNum* res, const mjtNum* vec, int n);

// Set res = res + vec*scl.
MJAPI void mju_addToScl(mjtNum* res, const mjtNum* vec, mjtNum scl, int n);

// Set res = vec1 + vec2*scl.
MJAPI void mju_addScl(mjtNum* res, const mjtNum* vec1, const mjtNum* vec2, mjtNum scl, int n);

// Normalize vector, return length before normalization.
MJAPI mjtNum mju_normalize(mjtNum* res, int n);

// Return vector length (without normalizing vector).
MJAPI mjtNum mju_norm(const mjtNum* res, int n);

// Return dot-product of vec1 and vec2.
MJAPI mjtNum mju_dot(const mjtNum* vec1, const mjtNum* vec2, int n);

// Multiply matrix and vector: res = mat * vec.
MJAPI void mju_mulMatVec(mjtNum* res, const mjtNum* mat, const mjtNum* vec, int nr, int nc);

// Multiply transposed matrix and vector: res = mat' * vec.
MJAPI void mju_mulMatTVec(mjtNum* res, const mjtNum* mat, const mjtNum* vec, int nr, int nc);

// Multiply square matrix with vectors on both sides: returns vec1' * mat * vec2.
MJAPI mjtNum mju_mulVecMatVec(const mjtNum* vec1, const mjtNum* mat, const mjtNum* vec2, int n);

// Transpose matrix: res = mat'.
MJAPI void mju_transpose(mjtNum* res, const mjtNum* mat, int nr, int nc);

// Symmetrize square matrix res = (mat + mat')/2.
MJAPI void mju_symmetrize(mjtNum* res, const mjtNum* mat, int n);

// Set mat to the identity matrix.
MJAPI void mju_eye(mjtNum* mat, int n);

// Multiply matrices: res = mat1 * mat2.
MJAPI void mju_mulMatMat(mjtNum* res, const mjtNum* mat1, const mjtNum* mat2,
                         int r1, int c1, int c2);

// Multiply matrices, second argument transposed: res = mat1 * mat2'.
MJAPI void mju_mulMatMatT(mjtNum* res, const mjtNum* mat1, const mjtNum* mat2,
                          int r1, int c1, int r2);

// Multiply matrices, first argument transposed: res = mat1' * mat2.
MJAPI void mju_mulMatTMat(mjtNum* res, const mjtNum* mat1, const mjtNum* mat2,
                          int r1, int c1, int c2);

// Set res = mat' * diag * mat if diag is not NULL, and res = mat' * mat otherwise.
MJAPI void mju_sqrMatTD(mjtNum* res, const mjtNum* mat, const mjtNum* diag, int nr, int nc);

// Coordinate transform of 6D motion or force vector in rotation:translation format.
// rotnew2old is 3-by-3, NULL means no rotation; flg_force specifies force or motion type.
MJAPI void mju_transformSpatial(mjtNum res[6], const mjtNum vec[6], int flg_force,
                                const mjtNum newpos[3], const mjtNum oldpos[3],
                                const mjtNum rotnew2old[9]);


//---------------------------------- Sparse math ---------------------------------------------------

// Convert matrix from dense to sparse.
//  nnz is size of res and colind, return 1 if too small, 0 otherwise.
MJAPI int mju_dense2sparse(mjtNum* res, const mjtNum* mat, int nr, int nc,
                           int* rownnz, int* rowadr, int* colind, int nnz);

// Convert matrix from sparse to dense.
MJAPI void mju_sparse2dense(mjtNum* res, const mjtNum* mat, int nr, int nc,
                            const int* rownnz, const int* rowadr, const int* colind);


//---------------------------------- Quaternions ---------------------------------------------------

// Rotate vector by quaternion.
MJAPI void mju_rotVecQuat(mjtNum res[3], const mjtNum vec[3], const mjtNum quat[4]);

// Conjugate quaternion, corresponding to opposite rotation.
MJAPI void mju_negQuat(mjtNum res[4], const mjtNum quat[4]);

// Multiply quaternions.
MJAPI void mju_mulQuat(mjtNum res[4], const mjtNum quat1[4], const mjtNum quat2[4]);

// Multiply quaternion and axis.
MJAPI void mju_mulQuatAxis(mjtNum res[4], const mjtNum quat[4], const mjtNum axis[3]);

// Convert axisAngle to quaternion.
MJAPI void mju_axisAngle2Quat(mjtNum res[4], const mjtNum axis[3], mjtNum angle);

// Convert quaternion (corresponding to orientation difference) to 3D velocity.
MJAPI void mju_quat2Vel(mjtNum res[3], const mjtNum quat[4], mjtNum dt);

// Subtract quaternions, express as 3D velocity: qb*quat(res) = qa.
MJAPI void mju_subQuat(mjtNum res[3], const mjtNum qa[4], const mjtNum qb[4]);

// Convert quaternion to 3D rotation matrix.
MJAPI void mju_quat2Mat(mjtNum res[9], const mjtNum quat[4]);

// Convert 3D rotation matrix to quaternion.
MJAPI void mju_mat2Quat(mjtNum quat[4], const mjtNum mat[9]);

// Compute time-derivative of quaternion, given 3D rotational velocity.
MJAPI void mju_derivQuat(mjtNum res[4], const mjtNum quat[4], const mjtNum vel[3]);

// Integrate quaternion given 3D angular velocity.
MJAPI void mju_quatIntegrate(mjtNum quat[4], const mjtNum vel[3], mjtNum scale);

// Construct quaternion performing rotation from z-axis to given vector.
MJAPI void mju_quatZ2Vec(mjtNum quat[4], const mjtNum vec[3]);

// Extract 3D rotation from an arbitrary 3x3 matrix by refining the input quaternion.
// Returns the number of iterations required to converge
MJAPI int mju_mat2Rot(mjtNum quat[4], const mjtNum mat[9]);

// Convert sequence of Euler angles (radians) to quaternion.
// seq[0,1,2] must be in 'xyzXYZ', lower/upper-case mean intrinsic/extrinsic rotations.
MJAPI void mju_euler2Quat(mjtNum quat[4], const mjtNum euler[3], const char* seq);


//---------------------------------- Poses ---------------------------------------------------------

// Multiply two poses.
MJAPI void mju_mulPose(mjtNum posres[3], mjtNum quatres[4],
                       const mjtNum pos1[3], const mjtNum quat1[4],
                       const mjtNum pos2[3], const mjtNum quat2[4]);

// Conjugate pose, corresponding to the opposite spatial transformation.
MJAPI void mju_negPose(mjtNum posres[3], mjtNum quatres[4],
                       const mjtNum pos[3], const mjtNum quat[4]);

// Transform vector by pose.
MJAPI void mju_trnVecPose(mjtNum res[3], const mjtNum pos[3], const mjtNum quat[4],
                          const mjtNum vec[3]);


//--------------------------------- Decompositions / Solvers ---------------------------------------

// Cholesky decomposition: mat = L*L'; return rank, decomposition performed in-place into mat.
MJAPI int mju_cholFactor(mjtNum* mat, int n, mjtNum mindiag);

// Solve (mat*mat') * res = vec, where mat is a Cholesky factor.
MJAPI void mju_cholSolve(mjtNum* res, const mjtNum* mat, const mjtNum* vec, int n);

// Cholesky rank-one update: L*L' +/- x*x'; return rank.
MJAPI int mju_cholUpdate(mjtNum* mat, mjtNum* x, int n, int flg_plus);

// Band-dense Cholesky decomposition.
//  Returns minimum value in the factorized diagonal, or 0 if rank-deficient.
//  mat has (ntotal-ndense) x nband + ndense x ntotal elements.
//  The first (ntotal-ndense) x nband store the band part, left of diagonal, inclusive.
//  The second ndense x ntotal store the band part as entire dense rows.
//  Add diagadd+diagmul*mat_ii to diagonal before factorization.
MJAPI mjtNum mju_cholFactorBand(mjtNum* mat, int ntotal, int nband, int ndense,
                                mjtNum diagadd, mjtNum diagmul);

// Solve (mat*mat')*res = vec where mat is a band-dense Cholesky factor.
MJAPI void mju_cholSolveBand(mjtNum* res, const mjtNum* mat, const mjtNum* vec,
                             int ntotal, int nband, int ndense);

// Convert banded matrix to dense matrix, fill upper triangle if flg_sym>0.
MJAPI void mju_band2Dense(mjtNum* res, const mjtNum* mat, int ntotal, int nband, int ndense,
                          mjtByte flg_sym);

// Convert dense matrix to banded matrix.
MJAPI void mju_dense2Band(mjtNum* res, const mjtNum* mat, int ntotal, int nband, int ndense);

// Multiply band-diagonal matrix with nvec vectors, include upper triangle if flg_sym>0.
MJAPI void mju_bandMulMatVec(mjtNum* res, const mjtNum* mat, const mjtNum* vec,
                             int ntotal, int nband, int ndense, int nvec, mjtByte flg_sym);

// Address of diagonal element i in band-dense matrix representation.
MJAPI int mju_bandDiag(int i, int ntotal, int nband, int ndense);

// Eigenvalue decomposition of symmetric 3x3 matrix, mat = eigvec * diag(eigval) * eigvec'.
MJAPI int mju_eig3(mjtNum eigval[3], mjtNum eigvec[9], mjtNum quat[4], const mjtNum mat[9]);

// minimize 0.5*x'*H*x + x'*g  s.t. lower <= x <= upper, return rank or -1 if failed
//   inputs:
//     n           - problem dimension
//     H           - SPD matrix                n*n
//     g           - bias vector               n
//     lower       - lower bounds              n
//     upper       - upper bounds              n
//     res         - solution warmstart        n
//   return value:
//     nfree <= n  - rank of unconstrained subspace, -1 if failure
//   outputs (required):
//     res         - solution                  n
//     R           - subspace Cholesky factor  nfree*nfree    allocated: n*(n+7)
//   outputs (optional):
//     index       - set of free dimensions    nfree          allocated: n
//   notes:
//     the initial value of res is used to warmstart the solver
//     R must have allocatd size n*(n+7), but only nfree*nfree values are used in output
//     index (if given) must have allocated size n, but only nfree values are used in output
//     only the lower triangles of H and R and are read from and written to, respectively
//     the convenience function mju_boxQPmalloc allocates the required data structures
MJAPI int mju_boxQP(mjtNum* res, mjtNum* R, int* index, const mjtNum* H, const mjtNum* g, int n,
                    const mjtNum* lower, const mjtNum* upper);

// allocate heap memory for box-constrained Quadratic Program
//   as in mju_boxQP, index, lower, and upper are optional
//   free all pointers with mju_free()
MJAPI void mju_boxQPmalloc(mjtNum** res, mjtNum** R, int** index, mjtNum** H, mjtNum** g, int n,
                           mjtNum** lower, mjtNum** upper);


//---------------------------------- Miscellaneous -------------------------------------------------

// Muscle active force, prm = (range[2], force, scale, lmin, lmax, vmax, fpmax, fvmax).
MJAPI mjtNum mju_muscleGain(mjtNum len, mjtNum vel, const mjtNum lengthrange[2],
                            mjtNum acc0, const mjtNum prm[9]);

// Muscle passive force, prm = (range[2], force, scale, lmin, lmax, vmax, fpmax, fvmax).
MJAPI mjtNum mju_muscleBias(mjtNum len, const mjtNum lengthrange[2],
                            mjtNum acc0, const mjtNum prm[9]);

// Muscle activation dynamics, prm = (tau_act, tau_deact, smoothing_width).
MJAPI mjtNum mju_muscleDynamics(mjtNum ctrl, mjtNum act, const mjtNum prm[3]);

// Convert contact force to pyramid representation.
MJAPI void mju_encodePyramid(mjtNum* pyramid, const mjtNum* force, const mjtNum* mu, int dim);

// Convert pyramid representation to contact force.
MJAPI void mju_decodePyramid(mjtNum* force, const mjtNum* pyramid, const mjtNum* mu, int dim);

// Integrate spring-damper analytically, return pos(dt).
MJAPI mjtNum mju_springDamper(mjtNum pos0, mjtNum vel0, mjtNum Kp, mjtNum Kv, mjtNum dt);

// Return min(a,b) with single evaluation of a and b.
MJAPI mjtNum mju_min(mjtNum a, mjtNum b);

// Return max(a,b) with single evaluation of a and b.
MJAPI mjtNum mju_max(mjtNum a, mjtNum b);

// Clip x to the range [min, max].
MJAPI mjtNum mju_clip(mjtNum x, mjtNum min, mjtNum max);

// Return sign of x: +1, -1 or 0.
MJAPI mjtNum mju_sign(mjtNum x);

// Round x to nearest integer.
MJAPI int mju_round(mjtNum x);

// Convert type id (mjtObj) to type name.
MJAPI const char* mju_type2Str(int type);

// Convert type name to type id (mjtObj).
MJAPI int mju_str2Type(const char* str);

// Return human readable number of bytes using standard letter suffix.
MJAPI const char* mju_writeNumBytes(size_t nbytes);

// Construct a warning message given the warning type and info.
MJAPI const char* mju_warningText(int warning, size_t info);

// Return 1 if nan or abs(x)>mjMAXVAL, 0 otherwise. Used by check functions.
MJAPI int mju_isBad(mjtNum x);

// Return 1 if all elements are 0.
MJAPI int mju_isZero(mjtNum* vec, int n);

// Standard normal random number generator (optional second number).
MJAPI mjtNum mju_standardNormal(mjtNum* num2);

// Convert from float to mjtNum.
MJAPI void mju_f2n(mjtNum* res, const float* vec, int n);

// Convert from mjtNum to float.
MJAPI void mju_n2f(float* res, const mjtNum* vec, int n);

// Convert from double to mjtNum.
MJAPI void mju_d2n(mjtNum* res, const double* vec, int n);

// Convert from mjtNum to double.
MJAPI void mju_n2d(double* res, const mjtNum* vec, int n);

// Insertion sort, resulting list is in increasing order.
MJAPI void mju_insertionSort(mjtNum* list, int n);

// Integer insertion sort, resulting list is in increasing order.
MJAPI void mju_insertionSortInt(int* list, int n);

// Generate Halton sequence.
MJAPI mjtNum mju_Halton(int index, int base);

// Call strncpy, then set dst[n-1] = 0.
MJAPI char* mju_strncpy(char *dst, const char *src, int n);

// Sigmoid function over 0<=x<=1 using quintic polynomial.
MJAPI mjtNum mju_sigmoid(mjtNum x);


//---------------------------------- Signed Distance Function --------------------------------------

// get sdf from geom id
MJAPI const mjpPlugin* mjc_getSDF(const mjModel* m, int id);

// signed distance function
MJAPI mjtNum mjc_distance(const mjModel* m, const mjData* d, const mjSDF* s, const mjtNum x[3]);

// gradient of sdf
MJAPI void mjc_gradient(const mjModel* m, const mjData* d, const mjSDF* s, mjtNum gradient[3],
                        const mjtNum x[3]);


//---------------------------------- Derivatives ---------------------------------------------------

// Finite differenced transition matrices (control theory notation)
//   d(x_next) = A*dx + B*du
//   d(sensor) = C*dx + D*du
//   required output matrix dimensions:
//      A: (2*nv+na x 2*nv+na)
//      B: (2*nv+na x nu)
//      D: (nsensordata x 2*nv+na)
//      C: (nsensordata x nu)
MJAPI void mjd_transitionFD(const mjModel* m, mjData* d, mjtNum eps, mjtByte flg_centered,
                            mjtNum* A, mjtNum* B, mjtNum* C, mjtNum* D);

// Finite differenced Jacobians of (force, sensors) = mj_inverse(state, acceleration)
//   All outputs are optional. Output dimensions (transposed w.r.t Control Theory convention):
//     DfDq: (nv x nv)
//     DfDv: (nv x nv)
//     DfDa: (nv x nv)
//     DsDq: (nv x nsensordata)
//     DsDv: (nv x nsensordata)
//     DsDa: (nv x nsensordata)
//     DmDq: (nv x nM)
//   single-letter shortcuts:
//     inputs: q=qpos, v=qvel, a=qacc
//     outputs: f=qfrc_inverse, s=sensordata, m=qM
//   notes:
//     optionally computes mass matrix Jacobian DmDq
//     flg_actuation specifies whether to subtract qfrc_actuator from qfrc_inverse
MJAPI void mjd_inverseFD(const mjModel* m, mjData* d, mjtNum eps, mjtByte flg_actuation,
                         mjtNum *DfDq, mjtNum *DfDv, mjtNum *DfDa,
                         mjtNum *DsDq, mjtNum *DsDv, mjtNum *DsDa,
                         mjtNum *DmDq);

// Derivatives of mju_subQuat.
MJAPI void mjd_subQuat(const mjtNum qa[4], const mjtNum qb[4], mjtNum Da[9], mjtNum Db[9]);

// Derivatives of mju_quatIntegrate.
MJAPI void mjd_quatIntegrate(const mjtNum vel[3], mjtNum scale,
                             mjtNum Dquat[9], mjtNum Dvel[9], mjtNum Dscale[3]);


//---------------------------------- Plugins -------------------------------------------------------

// Set default plugin definition.
MJAPI void mjp_defaultPlugin(mjpPlugin* plugin);

// Globally register a plugin. This function is thread-safe.
// If an identical mjpPlugin is already registered, this function does nothing.
// If a non-identical mjpPlugin with the same name is already registered, an mju_error is raised.
// Two mjpPlugins are considered identical if all member function pointers and numbers are equal,
// and the name and attribute strings are all identical, however the char pointers to the strings
// need not be the same.
MJAPI int mjp_registerPlugin(const mjpPlugin* plugin);

// Return the number of globally registered plugins.
MJAPI int mjp_pluginCount(void);

// Look up a plugin by name. If slot is not NULL, also write its registered slot number into it.
MJAPI const mjpPlugin* mjp_getPlugin(const char* name, int* slot);

// Look up a plugin by the registered slot number that was returned by mjp_registerPlugin.
MJAPI const mjpPlugin* mjp_getPluginAtSlot(int slot);

// Set default resource provider definition.
MJAPI void mjp_defaultResourceProvider(mjpResourceProvider* provider);

// Globally register a resource provider in a thread-safe manner. The provider must have a prefix
// that is not a sub-prefix or super-prefix of any current registered providers.  This function
// returns a slot number > 0 on success.
MJAPI int mjp_registerResourceProvider(const mjpResourceProvider* provider);

// Return the number of globally registered resource providers.
MJAPI int mjp_resourceProviderCount(void);

// Return the resource provider with the prefix that matches against the resource name.
// If no match, return NULL.
MJAPI const mjpResourceProvider* mjp_getResourceProvider(const char* resource_name);

// Look up a resource provider by slot number returned by mjp_registerResourceProvider.
// If invalid slot number, return NULL.
MJAPI const mjpResourceProvider* mjp_getResourceProviderAtSlot(int slot);


//---------------------------------- Threads -------------------------------------------------------

// Create a thread pool with the specified number of threads running.
MJAPI mjThreadPool* mju_threadPoolCreate(size_t number_of_threads);

// Adds a thread pool to mjData and configures it for multi-threaded use.
MJAPI void mju_bindThreadPool(mjData* d, void* thread_pool);

// Enqueue a task in a thread pool.
MJAPI void mju_threadPoolEnqueue(mjThreadPool* thread_pool, mjTask* task);

// Destroy a thread pool.
MJAPI void mju_threadPoolDestroy(mjThreadPool* thread_pool);

// Initialize an mjTask.
MJAPI void mju_defaultTask(mjTask* task);

// Wait for a task to complete.
MJAPI void mju_taskJoin(mjTask* task);


//---------------------------------- Attachment ----------------------------------------------------

// Attach child to a parent, return the attached element if success or NULL otherwise.
MJAPI mjsElement* mjs_attach(mjsElement* parent, const mjsElement* child,
                             const char* prefix, const char* suffix);


//---------------------------------- Tree elements -------------------------------------------------

// Add child body to body, return child.
MJAPI mjsBody* mjs_addBody(mjsBody* body, const mjsDefault* def);

// Add site to body, return site spec.
MJAPI mjsSite* mjs_addSite(mjsBody* body, const mjsDefault* def);

// Add joint to body.
MJAPI mjsJoint* mjs_addJoint(mjsBody* body, const mjsDefault* def);

// Add freejoint to body.
MJAPI mjsJoint* mjs_addFreeJoint(mjsBody* body);

// Add geom to body.
MJAPI mjsGeom* mjs_addGeom(mjsBody* body, const mjsDefault* def);

// Add camera to body.
MJAPI mjsCamera* mjs_addCamera(mjsBody* body, const mjsDefault* def);

// Add light to body.
MJAPI mjsLight* mjs_addLight(mjsBody* body, const mjsDefault* def);

// Add frame to body.
MJAPI mjsFrame* mjs_addFrame(mjsBody* body, mjsFrame* parentframe);

// Remove object corresponding to the given element, return 0 on success.
MJAPI int mjs_delete(mjSpec* spec, mjsElement* element);


//---------------------------------- Non-tree elements ---------------------------------------------

// Add actuator.
MJAPI mjsActuator* mjs_addActuator(mjSpec* s, const mjsDefault* def);

// Add sensor.
MJAPI mjsSensor* mjs_addSensor(mjSpec* s);

// Add flex.
MJAPI mjsFlex* mjs_addFlex(mjSpec* s);

// Add contact pair.
MJAPI mjsPair* mjs_addPair(mjSpec* s, const mjsDefault* def);

// Add excluded body pair.
MJAPI mjsExclude* mjs_addExclude(mjSpec* s);

// Add equality.
MJAPI mjsEquality* mjs_addEquality(mjSpec* s, const mjsDefault* def);

// Add tendon.
MJAPI mjsTendon* mjs_addTendon(mjSpec* s, const mjsDefault* def);

// Wrap site using tendon.
MJAPI mjsWrap* mjs_wrapSite(mjsTendon* tendon, const char* name);

// Wrap geom using tendon.
MJAPI mjsWrap* mjs_wrapGeom(mjsTendon* tendon, const char* name, const char* sidesite);

// Wrap joint using tendon.
MJAPI mjsWrap* mjs_wrapJoint(mjsTendon* tendon, const char* name, double coef);

// Wrap pulley using tendon.
MJAPI mjsWrap* mjs_wrapPulley(mjsTendon* tendon, double divisor);

// Add numeric.
MJAPI mjsNumeric* mjs_addNumeric(mjSpec* s);

// Add text.
MJAPI mjsText* mjs_addText(mjSpec* s);

// Add tuple.
MJAPI mjsTuple* mjs_addTuple(mjSpec* s);

// Add keyframe.
MJAPI mjsKey* mjs_addKey(mjSpec* s);

// Add plugin.
MJAPI mjsPlugin* mjs_addPlugin(mjSpec* s);

// Add default.
MJAPI mjsDefault* mjs_addDefault(mjSpec* s, const char* classname, const mjsDefault* parent);


//---------------------------------- Set actuator parameters ---------------------------------------

// Set actuator to motor, return error if any.
MJAPI const char* mjs_setToMotor(mjsActuator* actuator);

// Set actuator to position, return error if any.
MJAPI const char* mjs_setToPosition(mjsActuator* actuator, double kp, double kv[1],
                                    double dampratio[1], double timeconst[1], double inheritrange);

// Set actuator to integrated velocity, return error if any.
MJAPI const char* mjs_setToIntVelocity(mjsActuator* actuator, double kp, double kv[1],
                                       double dampratio[1], double timeconst[1], double inheritrange);

// Set actuator to velocity servo, return error if any.
MJAPI const char* mjs_setToVelocity(mjsActuator* actuator, double kv);

// Set actuator to activate damper, return error if any.
MJAPI const char* mjs_setToDamper(mjsActuator* actuator, double kv);

// Set actuator to hydraulic or pneumatic cylinder, return error if any.
MJAPI const char* mjs_setToCylinder(mjsActuator* actuator, double timeconst,
                                    double bias, double area, double diameter);

// Set actuator to muscle, return error if any.a
MJAPI const char* mjs_setToMuscle(mjsActuator* actuator, double timeconst[2], double tausmooth,
                                  double range[2], double force, double scale, double lmin,
                                  double lmax, double vmax, double fpmax, double fvmax);

// Set actuator to active adhesion, return error if any.
MJAPI const char* mjs_setToAdhesion(mjsActuator* actuator, double gain);


//---------------------------------- Assets --------------------------------------------------------

// Add mesh.
MJAPI mjsMesh* mjs_addMesh(mjSpec* s, const mjsDefault* def);

// Add height field.
MJAPI mjsHField* mjs_addHField(mjSpec* s);

// Add skin.
MJAPI mjsSkin* mjs_addSkin(mjSpec* s);

// Add texture.
MJAPI mjsTexture* mjs_addTexture(mjSpec* s);

// Add material.
MJAPI mjsMaterial* mjs_addMaterial(mjSpec* s, const mjsDefault* def);


//---------------------------------- Find and get utilities ----------------------------------------

// Get spec from body.
MJAPI mjSpec* mjs_getSpec(mjsElement* element);

// Find spec (model asset) by name.
MJAPI mjSpec* mjs_findSpec(mjSpec* spec, const char* name);

// Find body in spec by name.
MJAPI mjsBody* mjs_findBody(mjSpec* s, const char* name);

// Find element in spec by name.
MJAPI mjsElement* mjs_findElement(mjSpec* s, mjtObj type, const char* name);

// Find child body by name.
MJAPI mjsBody* mjs_findChild(mjsBody* body, const char* name);

// Get parent body.
MJAPI mjsBody* mjs_getParent(mjsElement* element);

// Get parent frame.
MJAPI mjsFrame* mjs_getFrame(mjsElement* element);

// Find frame by name.
MJAPI mjsFrame* mjs_findFrame(mjSpec* s, const char* name);

// Get default corresponding to an element.
MJAPI mjsDefault* mjs_getDefault(mjsElement* element);

// Find default in model by class name.
MJAPI mjsDefault* mjs_findDefault(mjSpec* s, const char* classname);

// Get global default from model.
MJAPI mjsDefault* mjs_getSpecDefault(mjSpec* s);

// Get element id.
MJAPI int mjs_getId(mjsElement* element);

// Return body's first child of given type. If recurse is nonzero, also search the body's subtree.
MJAPI mjsElement* mjs_firstChild(mjsBody* body, mjtObj type, int recurse);

// Return body's next child of the same type; return NULL if child is last.
// If recurse is nonzero, also search the body's subtree.
MJAPI mjsElement* mjs_nextChild(mjsBody* body, mjsElement* child, int recurse);

// Return spec's first element of selected type.
MJAPI mjsElement* mjs_firstElement(mjSpec* s, mjtObj type);

// Return spec's next element; return NULL if element is last.
MJAPI mjsElement* mjs_nextElement(mjSpec* s, mjsElement* element);


//---------------------------------- Attribute setters ---------------------------------------------

// Set element's name, return 0 on success.
MJAPI int mjs_setName(mjsElement* element, const char* name);

// Copy buffer.
MJAPI void mjs_setBuffer(mjByteVec* dest, const void* array, int size);

// Copy text to string.
MJAPI void mjs_setString(mjString* dest, const char* text);

// Split text to entries and copy to string vector.
MJAPI void mjs_setStringVec(mjStringVec* dest, const char* text);

// Set entry in string vector.
MJAPI mjtByte mjs_setInStringVec(mjStringVec* dest, int i, const char* text);

// Append text entry to string vector.
MJAPI void mjs_appendString(mjStringVec* dest, const char* text);

// Copy int array to vector.
MJAPI void mjs_setInt(mjIntVec* dest, const int* array, int size);

// Append int array to vector of arrays.
MJAPI void mjs_appendIntVec(mjIntVecVec* dest, const int* array, int size);

// Copy float array to vector.
MJAPI void mjs_setFloat(mjFloatVec* dest, const float* array, int size);

// Append float array to vector of arrays.
MJAPI void mjs_appendFloatVec(mjFloatVecVec* dest, const float* array, int size);

// Copy double array to vector.
MJAPI void mjs_setDouble(mjDoubleVec* dest, const double* array, int size);

// Set plugin attributes.
MJAPI void mjs_setPluginAttributes(mjsPlugin* plugin, void* attributes);


//---------------------------------- Attribute getters ---------------------------------------------

// Get element's name.
MJAPI mjString* mjs_getName(mjsElement* element);

// Get string contents.
MJAPI const char* mjs_getString(const mjString* source);

// Get double array contents and optionally its size.
MJAPI const double* mjs_getDouble(const mjDoubleVec* source, int* size);

// Get plugin attributes.
MJAPI const void* mjs_getPluginAttributes(const mjsPlugin* plugin);


//---------------------------------- Spec utilities ------------------------------------------------

// Set element's default.
MJAPI void mjs_setDefault(mjsElement* element, const mjsDefault* def);

// Set element's enclosing frame, return 0 on success.
MJAPI int mjs_setFrame(mjsElement* dest, mjsFrame* frame);

// Resolve alternative orientations to quat, return error if any.
MJAPI const char* mjs_resolveOrientation(double quat[4], mjtByte degree, const char* sequence,
                                         const mjsOrientation* orientation);

// Transform body into a frame.
MJAPI mjsFrame* mjs_bodyToFrame(mjsBody** body);

// Set user payload, overriding the existing value for the specified key if present.
MJAPI void mjs_setUserValue(mjsElement* element, const char* key, const void* data);

// Set user payload, overriding the existing value for the specified key if
// present. This version differs from mjs_setUserValue in that it takes a
// cleanup function that will be called when the user payload is deleted.
MJAPI void mjs_setUserValueWithCleanup(mjsElement* element, const char* key,
                                       const void* data,
                                       void (*cleanup)(const void*));

// Return user payload or NULL if none found.
MJAPI const void* mjs_getUserValue(mjsElement* element, const char* key);

// Delete user payload.
MJAPI void mjs_deleteUserValue(mjsElement* element, const char* key);

//---------------------------------- Element initialization  ---------------------------------------

// Default spec attributes.
MJAPI void mjs_defaultSpec(mjSpec* spec);

// Default orientation attributes.
MJAPI void mjs_defaultOrientation(mjsOrientation* orient);

// Default body attributes.
MJAPI void mjs_defaultBody(mjsBody* body);

// Default frame attributes.
MJAPI void mjs_defaultFrame(mjsFrame* frame);

// Default joint attributes.
MJAPI void mjs_defaultJoint(mjsJoint* joint);

// Default geom attributes.
MJAPI void mjs_defaultGeom(mjsGeom* geom);

// Default site attributes.
MJAPI void mjs_defaultSite(mjsSite* site);

// Default camera attributes.
MJAPI void mjs_defaultCamera(mjsCamera* camera);

// Default light attributes.
MJAPI void mjs_defaultLight(mjsLight* light);

// Default flex attributes.
MJAPI void mjs_defaultFlex(mjsFlex* flex);

// Default mesh attributes.
MJAPI void mjs_defaultMesh(mjsMesh* mesh);

// Default height field attributes.
MJAPI void mjs_defaultHField(mjsHField* hfield);

// Default skin attributes.
MJAPI void mjs_defaultSkin(mjsSkin* skin);

// Default texture attributes.
MJAPI void mjs_defaultTexture(mjsTexture* texture);

// Default material attributes.
MJAPI void mjs_defaultMaterial(mjsMaterial* material);

// Default pair attributes.
MJAPI void mjs_defaultPair(mjsPair* pair);

// Default equality attributes.
MJAPI void mjs_defaultEquality(mjsEquality* equality);

// Default tendon attributes.
MJAPI void mjs_defaultTendon(mjsTendon* tendon);

// Default actuator attributes.
MJAPI void mjs_defaultActuator(mjsActuator* actuator);

// Default sensor attributes.
MJAPI void mjs_defaultSensor(mjsSensor* sensor);

// Default numeric attributes.
MJAPI void mjs_defaultNumeric(mjsNumeric* numeric);

// Default text attributes.
MJAPI void mjs_defaultText(mjsText* text);

// Default tuple attributes.
MJAPI void mjs_defaultTuple(mjsTuple* tuple);

// Default keyframe attributes.
MJAPI void mjs_defaultKey(mjsKey* key);

// Default plugin attributes.
MJAPI void mjs_defaultPlugin(mjsPlugin* plugin);


//---------------------------------- Element casting -----------------------------------------------

// Safely cast an element as mjsBody, or return NULL if the element is not an mjsBody.
MJAPI mjsBody* mjs_asBody(mjsElement* element);

// Safely cast an element as mjsGeom, or return NULL if the element is not an mjsGeom.
MJAPI mjsGeom* mjs_asGeom(mjsElement* element);

// Safely cast an element as mjsJoint, or return NULL if the element is not an mjsJoint.
MJAPI mjsJoint* mjs_asJoint(mjsElement* element);

// Safely cast an element as mjsSite, or return NULL if the element is not an mjsSite.
MJAPI mjsSite* mjs_asSite(mjsElement* element);

// Safely cast an element as mjsCamera, or return NULL if the element is not an mjsCamera.
MJAPI mjsCamera* mjs_asCamera(mjsElement* element);

// Safely cast an element as mjsLight, or return NULL if the element is not an mjsLight.
MJAPI mjsLight* mjs_asLight(mjsElement* element);

// Safely cast an element as mjsFrame, or return NULL if the element is not an mjsFrame.
MJAPI mjsFrame* mjs_asFrame(mjsElement* element);

// Safely cast an element as mjsActuator, or return NULL if the element is not an mjsActuator.
MJAPI mjsActuator* mjs_asActuator(mjsElement* element);

// Safely cast an element as mjsSensor, or return NULL if the element is not an mjsSensor.
MJAPI mjsSensor* mjs_asSensor(mjsElement* element);

// Safely cast an element as mjsFlex, or return NULL if the element is not an mjsFlex.
MJAPI mjsFlex* mjs_asFlex(mjsElement* element);

// Safely cast an element as mjsPair, or return NULL if the element is not an mjsPair.
MJAPI mjsPair* mjs_asPair(mjsElement* element);

// Safely cast an element as mjsEquality, or return NULL if the element is not an mjsEquality.
MJAPI mjsEquality* mjs_asEquality(mjsElement* element);

// Safely cast an element as mjsExclude, or return NULL if the element is not an mjsExclude.
MJAPI mjsExclude* mjs_asExclude(mjsElement* element);

// Safely cast an element as mjsTendon, or return NULL if the element is not an mjsTendon.
MJAPI mjsTendon* mjs_asTendon(mjsElement* element);

// Safely cast an element as mjsNumeric, or return NULL if the element is not an mjsNumeric.
MJAPI mjsNumeric* mjs_asNumeric(mjsElement* element);

// Safely cast an element as mjsText, or return NULL if the element is not an mjsText.
MJAPI mjsText* mjs_asText(mjsElement* element);

// Safely cast an element as mjsTuple, or return NULL if the element is not an mjsTuple.
MJAPI mjsTuple* mjs_asTuple(mjsElement* element);

// Safely cast an element as mjsKey, or return NULL if the element is not an mjsKey.
MJAPI mjsKey* mjs_asKey(mjsElement* element);

// Safely cast an element as mjsMesh, or return NULL if the element is not an mjsMesh.
MJAPI mjsMesh* mjs_asMesh(mjsElement* element);

// Safely cast an element as mjsHField, or return NULL if the element is not an mjsHField.
MJAPI mjsHField* mjs_asHField(mjsElement* element);

// Safely cast an element as mjsSkin, or return NULL if the element is not an mjsSkin.
MJAPI mjsSkin* mjs_asSkin(mjsElement* element);

// Safely cast an element as mjsTexture, or return NULL if the element is not an mjsTexture.
MJAPI mjsTexture* mjs_asTexture(mjsElement* element);

// Safely cast an element as mjsMaterial, or return NULL if the element is not an mjsMaterial.
MJAPI mjsMaterial* mjs_asMaterial(mjsElement* element);

// Safely cast an element as mjsPlugin, or return NULL if the element is not an mjsPlugin.
MJAPI mjsPlugin* mjs_asPlugin(mjsElement* element);

#ifdef __cplusplus
}
#endif

#endif  // MUJOCO_MUJOCO_H_
