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

// Tests for engine/engine_core_smooth.c.

#include "src/engine/engine_core_smooth.h"
#include "src/engine/engine_util_sparse.h"

#include <string>
#include <string_view>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <absl/types/span.h>
#include <mujoco/mjmodel.h>
#include <mujoco/mjxmacro.h>
#include <mujoco/mujoco.h>
#include "test/fixture.h"

namespace mujoco {
namespace {

using ::std::vector;
using ::testing::Each;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Pointwise;
using ::testing::DoubleNear;
using ::testing::NotNull;
using CoreSmoothTest = MujocoTest;


constexpr bool EndsWith(std::string_view str, std::string_view suffix) {
  return str.size() >= suffix.size() &&
         str.substr(str.size() - suffix.size()) == suffix;
}

// mjData values corresponding to the world body should be zero or identity
TEST_F(CoreSmoothTest, MjDataWorldBodyValuesAreInitialized) {
  constexpr char xml[] = R"(
  <mujoco>
    <option>
      <flag gravity="disable"/>
    </option>
    <worldbody/>
    <sensor>
      <subtreelinvel body="world"/>
    </sensor>
  </mujoco>
  )";
  mjModel* model = LoadModelFromString(xml);
  ASSERT_THAT(model, NotNull());
  mjData* data = mj_makeData(model);
  mj_resetDataDebug(model, data, 'd');
  mj_forward(model, data);
  mj_rnePostConstraint(model, data);

  {
    MJDATA_POINTERS_PREAMBLE(model)
#define X(type, name, d0, d1)                                                 \
    if constexpr (std::string_view(#d0) == "nbody") {                         \
      absl::Span<type> values(data->name, model->d0 * d1);                    \
      if constexpr (EndsWith(#name, "quat")) {                                \
        EXPECT_THAT(values, ElementsAre(1, 0, 0, 0)) << #name;                \
      } else if constexpr (EndsWith(#name, "mat")) {                          \
        EXPECT_THAT(values, ElementsAre(1, 0, 0, 0, 1, 0, 0, 0, 1)) << #name; \
      } else {                                                                \
        EXPECT_THAT(values, Each(Eq(0))) << #name;                            \
      }                                                                       \
    }
    MJDATA_POINTERS
#undef X
  }

  mj_deleteData(data);
  mj_deleteModel(model);
}

// --------------------------- mj_kinematics -----------------------------------

TEST_F(CoreSmoothTest, MjKinematicsWorldXipos) {
  constexpr char xml[] = R"(
  <mujoco>
    <worldbody>
    </worldbody>
  </mujoco>
  )";
  mjModel* model = LoadModelFromString(xml);
  ASSERT_THAT(model, NotNull());
  mjData* data = mj_makeData(model);

  mj_resetDataDebug(model, data, 'd');
  mj_kinematics(model, data);
  EXPECT_THAT(AsVector(&data->xipos[0], 3), ElementsAre(0, 0, 0));

  mj_deleteData(data);
  mj_deleteModel(model);
}

// ----------------------------- mj_tendon -------------------------------------

TEST_F(CoreSmoothTest, FixedTendonSortedIndices) {
  constexpr char xml[] = R"(
  <mujoco>
    <option jacobian="sparse"/>

    <worldbody>
      <body>
        <geom size=".1"/>
        <joint name="0"/>
      </body>
      <body pos="1 0 0">
        <geom size=".1"/>
        <joint name="1"/>
      </body>
      <body pos="2 0 0">
        <geom size=".1"/>
        <joint name="2"/>
      </body>
    </worldbody>

    <tendon>
      <fixed>
        <joint coef="3" joint="2"/>
        <joint coef="2" joint="1"/>
        <joint coef="1" joint="0"/>
      </fixed>
    </tendon>
  </mujoco>
  )";
  mjModel* model = LoadModelFromString(xml);
  ASSERT_THAT(model, NotNull());
  ASSERT_EQ(model->ntendon, 1);
  ASSERT_EQ(model->nwrap, 3);

  mjData* data = mj_makeData(model);
  mj_fwdPosition(model, data);

  int rowadr = data->ten_J_rowadr[0];
  int* colind = data->ten_J_colind + rowadr;
  mjtNum* J = data->ten_J + rowadr;

  EXPECT_THAT(vector<mjtNum>(J, J + 3), ElementsAre(1, 2, 3));
  EXPECT_THAT(vector<int>(colind, colind + 3), ElementsAre(0, 1, 2));

  mj_deleteData(data);
  mj_deleteModel(model);
}

// --------------------------- connect constraint ------------------------------

// test that bodies hanging on connects lead to expected force sensor readings
void TestConnect(const char* const filepath) {
  const std::string xml_path = GetTestDataFilePath(filepath);
  mjModel* model =
      mj_loadXML(xml_path.c_str(), nullptr, 0, 0);
  mjData* data = mj_makeData(model);
  // settle physics:
  for (int i=0; i < 1000; i++) {
    mj_step(model, data);
  }
  for (int i=0; i < 3; i++) {
    EXPECT_NEAR(data->sensordata[i], model->sensor_user[i], 1e-6);
  }
  mj_deleteData(data);
  mj_deleteModel(model);
}


TEST_F(CoreSmoothTest, RnePostConnectForceSlide) {
  constexpr char kModelFilePath[] =
      "engine/testdata/core_smooth/rne_post/connect/force_slide.xml";
  TestConnect(kModelFilePath);
}


TEST_F(CoreSmoothTest, RnePostConnectForceSlideRotated) {
  constexpr char kModelFilePath[] =
      "engine/testdata/core_smooth/rne_post/connect/force_slide_rotated.xml";
  TestConnect(kModelFilePath);
}


TEST_F(CoreSmoothTest, RnePostConnectForceFree) {
  constexpr char kModelFilePath[] =
      "engine/testdata/core_smooth/rne_post/connect/force_free.xml";
  TestConnect(kModelFilePath);
}


TEST_F(CoreSmoothTest, RnePostConnectTorque) {
  constexpr char kModelFilePath[] =
      "engine/testdata/core_smooth/rne_post/connect/torque_free.xml";
  TestConnect(kModelFilePath);
}


TEST_F(CoreSmoothTest, RnePostConnectMultipleConstraints) {
  constexpr char kModelFilePath[] =
      "engine/testdata/core_smooth/rne_post/connect/multiple_constraints.xml";
  TestConnect(kModelFilePath);
}


// --------------------------- weld constraint ---------------------------------

// test that bodies attached with welds lead to expected force sensor readings
void TestWeld(const char* const filepath) {
  const std::string xml_path = GetTestDataFilePath(filepath);
  mjModel* model =
      mj_loadXML(xml_path.c_str(), nullptr, 0, 0);
  mjData* data = mj_makeData(model);
  // settle physics:
  for (int i=0; i < 1000; i++) {
    mj_step(model, data);
  }
  for (int sensor_index=0; sensor_index < model->nsensor; sensor_index++) {
    for (int i=0; i < 3; i++) {
      EXPECT_NEAR(
          data->sensordata[model->sensor_adr[sensor_index] + i],
          model->sensor_user[model->nuser_sensor*sensor_index + i],
          1e-6);
    }
  }
  mj_deleteData(data);
  mj_deleteModel(model);
}


TEST_F(CoreSmoothTest, RnePostWeldForceFree) {
  constexpr char kModelFilePath[] =
      "engine/testdata/core_smooth/rne_post/weld/force_free.xml";
  TestWeld(kModelFilePath);
}


TEST_F(CoreSmoothTest, RnePostWeldForceFreeRotated) {
  constexpr char kModelFilePath[] =
      "engine/testdata/core_smooth/rne_post/weld/force_free_rotated.xml";
  TestWeld(kModelFilePath);
}


TEST_F(CoreSmoothTest, RnePostWeldForceTorqueFree) {
  constexpr char kModelFilePath[] =
      "engine/testdata/core_smooth/rne_post/weld/force_torque_free.xml";
  TestWeld(kModelFilePath);
}


TEST_F(CoreSmoothTest, RnePostWeldForceTorqueFreeRotated) {
  constexpr char kModelFilePath[] =
      "engine/testdata/core_smooth/rne_post/weld/force_torque_free_rotated.xml";
  TestWeld(kModelFilePath);
}


TEST_F(CoreSmoothTest, WeldRatioForceFree) {
  constexpr char kModelFilePath[] =
      "engine/testdata/core_smooth/rne_post/weld/tfratio0_force_free.xml";
  TestConnect(kModelFilePath);
}


TEST_F(CoreSmoothTest, WeldRatioForceSlide) {
  constexpr char kModelFilePath[] =
      "engine/testdata/core_smooth/rne_post/weld/tfratio0_force_slide.xml";
  TestConnect(kModelFilePath);
}


TEST_F(CoreSmoothTest, WeldRatioTorqueFree) {
  constexpr char kModelFilePath[] =
      "engine/testdata/core_smooth/rne_post/weld/tfratio0_torque_free.xml";
  TestConnect(kModelFilePath);
}


TEST_F(CoreSmoothTest, WeldRatioForceSlideRotated) {
  constexpr char kModelFilePath[] =
      "engine/testdata/core_smooth/rne_post/weld/"
      "tfratio0_force_slide_rotated.xml";
  TestConnect(kModelFilePath);
}

TEST_F(CoreSmoothTest, WeldRatioMultipleConstraints) {
  constexpr char kModelFilePath[] =
      "engine/testdata/core_smooth/rne_post/weld/"
      "tfratio0_multiple_constraints.xml";
  TestConnect(kModelFilePath);
}

TEST_F(CoreSmoothTest, EqualityBodySite) {
  const std::string xml_path =
      GetTestDataFilePath("engine/testdata/equality_site_body_compare.xml");

  mjModel* model = mj_loadXML(xml_path.c_str(), nullptr, nullptr, 0);
  mjData* data = mj_makeData(model);

  // simulate, get sensordata
  while (data->time < 0.1) {
    mj_step(model, data);
  }
  vector<mjtNum> sdata = AsVector(data->sensordata, model->nsensordata);

  // reset
  mj_resetData(model, data);

  // turn site-defined equalities off, equivalent body-defined equalities on
  for (int e=0; e < 4; e++) data->eq_active[e] = 1 - data->eq_active[e];

  // simulate again, get sensordata
  while (data->time < 0.1) {
    mj_step(model, data);
  }

  // compare
  EXPECT_THAT(AsVector(data->sensordata, model->nsensordata),
              Pointwise(DoubleNear(1e-8), sdata));

  mj_deleteData(data);
  mj_deleteModel(model);
}

// --------------------------- site actuators ----------------------------------

// Test Cartesian position control using site transmission with refsite
TEST_F(CoreSmoothTest, RefsiteBringsToPose) {
  constexpr char kRefsitePath[] = "engine/testdata/actuation/refsite.xml";
  const std::string xml_path = GetTestDataFilePath(kRefsitePath);
  mjModel* model = mj_loadXML(xml_path.c_str(), nullptr, 0, 0);
  ASSERT_THAT(model, NotNull());
  mjData* data = mj_makeData(model);

  // set pose target in ctrl (3 positions, 3 rotations)
  mjtNum targetpos[] = {.01, .02, .03};
  mjtNum targetrot[] = {.1, .2, .3};
  mju_copy3(data->ctrl, targetpos);
  mju_copy3(data->ctrl+3, targetrot);

  // step for 5 seconds
  while (data->time < 10) {
    mj_step(model, data);
  }

  // get site IDs
  int refsite_id = mj_name2id(model, mjOBJ_SITE, "reference");
  int site_id = mj_name2id(model, mjOBJ_SITE, "end_effector");

  // check that position matches target to within 1e-3 length units
  double tol_pos = 1e-3;
  mjtNum relpos[3];
  mju_sub3(relpos, data->site_xpos+3*site_id, data->site_xpos+3*refsite_id);
  EXPECT_THAT(relpos, Pointwise(DoubleNear(tol_pos), targetpos));

  // check that orientation matches target to within 0.06 radians
  double tol_rot = 0.06;
  mjtNum site_xquat[4], refsite_xquat[4], relrot[3];
  mju_mat2Quat(refsite_xquat, data->site_xmat+9*refsite_id);
  mju_mat2Quat(site_xquat, data->site_xmat+9*site_id);
  mju_subQuat(relrot, site_xquat, refsite_xquat);
  EXPECT_THAT(relrot, Pointwise(DoubleNear(tol_rot), targetrot));

  mj_deleteData(data);
  mj_deleteModel(model);
}

// Test Cartesian position control w.r.t moving refsite
TEST_F(CoreSmoothTest, RefsiteConservesMomentum) {
  constexpr char kRefsitePath[] = "engine/testdata/actuation/refsite_free.xml";
  const std::string xml_path = GetTestDataFilePath(kRefsitePath);
  mjModel* model = mj_loadXML(xml_path.c_str(), nullptr, 0, 0);
  ASSERT_THAT(model, NotNull());
  mjData* data = mj_makeData(model);

  data->ctrl[0] = 1;
  data->ctrl[1] = -1;

  // simulate, assert that momentum is conserved
  mjtNum eps = 1e-9;
  while (data->time < 1) {
    mj_step(model, data);
    for (int i=0; i < 6; i++) {
      EXPECT_LT(mju_abs(data->sensordata[i]), eps);
    }
  }

  mj_deleteData(data);
  mj_deleteModel(model);
}

static const char* const kIlslandEfcPath =
    "engine/testdata/island/island_efc.xml";
static const char* const kModelPath =
    "testdata/model.xml";

TEST_F(CoreSmoothTest, SolveMIsland) {
  for (auto model_path : {kModelPath, kIlslandEfcPath}) {
    const std::string xml_path = GetTestDataFilePath(model_path);
    mjModel* model = mj_loadXML(xml_path.c_str(), nullptr, nullptr, 0);
    mjData* data = mj_makeData(model);
    int nv = model->nv;

    // allocate vec, fill with arbitrary values, copy to sol
    mjtNum* vec = (mjtNum*) mju_malloc(sizeof(mjtNum) * nv);
    mjtNum* res = (mjtNum*) mju_malloc(sizeof(mjtNum) * nv);
    for (int i=0; i < nv; i++) {
      vec[i] = 0.2 + 0.3*i;
    }
    mju_copy(res, vec, nv);

    if (model->nkey > 0) mj_resetDataKeyframe(model, data, 0);

    for (int i=0; i < 6; i++) {
      mj_step(model, data);
    }

    mj_forward(model, data);

    // divide by mass matrix: sol = M^-1 * vec
    mj_solveM(model, data, res, res, 1);

    // iterate over islands
    for (int i=0; i < data->nisland; i++) {
      // allocate dof vectors for island
      int dofnum = data->island_dofnum[i];
      mjtNum* res_i = (mjtNum*)mju_malloc(sizeof(mjtNum) * dofnum);

      // copy values into sol_i
      int* dofind = data->island_dofind + data->island_dofadr[i];
      for (int j=0; j < dofnum; j++) {
        res_i[j] = vec[dofind[j]];
      }

      // divide by mass matrix, for this island
      mj_solveM_island(model, data, res_i, i);

      // expect corresponding values to match
      for (int j=0; j < dofnum; j++) {
        EXPECT_THAT(res_i[j], DoubleNear(res[dofind[j]], 1e-12));
      }
      mju_free(res_i);
    }

    mju_free(res);
    mju_free(vec);
    mj_deleteData(data);
    mj_deleteModel(model);
  }
}

static const char* const kInertiaPath = "engine/testdata/inertia.xml";

TEST_F(CoreSmoothTest, FactorI) {
  const std::string xml_path = GetTestDataFilePath(kInertiaPath);
  char error[1024];
  mjModel* model = mj_loadXML(xml_path.c_str(), nullptr, error, sizeof(error));
  ASSERT_THAT(model, NotNull()) << "Failed to load model: " << error;

  mjData* data = mj_makeData(model);
  mj_forward(model, data);

  // dense L matrix
  int nv = model->nv;
  vector<mjtNum> Ldense(nv*nv, 0);
  mju_sparse2dense(Ldense.data(), data->qLD, nv, nv,
                   data->C_rownnz, data->C_rowadr, data->C_colind);
  for (int i=0; i < nv; i++) {
    // set diagonal to 1
    Ldense[i*nv+i] = 1;
  }

  // dense D matrix
  vector<mjtNum> Ddense(nv*nv);
  mju_sparse2dense(Ddense.data(), data->qLD, nv, nv,
                   data->C_rownnz, data->C_rowadr, data->C_colind);
  for (int i=0; i < nv; i++) {
    for (int j=0; j < nv; j++) {
      // zero everything except the diagonal
      if (i != j) Ddense[i*nv+j] = 0;
    }
  }

  // perform multiplication: M = L^T * D * L
  vector<mjtNum> tmp(nv*nv);
  vector<mjtNum> M(nv*nv);
  mju_mulMatMat(tmp.data(), Ddense.data(), Ldense.data(), nv, nv, nv);
  mju_mulMatTMat(M.data(), Ldense.data(), tmp.data(), nv, nv, nv);

  // dense M matrix
  vector<mjtNum> Mexpected(nv*nv);
  mj_fullM(model, Mexpected.data(), data->qM);

  // expect matrices to match to floating point precision
  EXPECT_THAT(M, Pointwise(DoubleNear(1e-12), Mexpected));

  mj_deleteData(data);
  mj_deleteModel(model);
}

TEST_F(CoreSmoothTest, SolveLDs) {
  const std::string xml_path = GetTestDataFilePath(kInertiaPath);
  char error[1024];
  mjModel* m = mj_loadXML(xml_path.c_str(), nullptr, error, sizeof(error));
  ASSERT_THAT(m, NotNull()) << "Failed to load model: " << error;

  mjData* d = mj_makeData(m);
  mj_forward(m, d);

  int nv = m->nv;
  int nM = m->nM;
  int nC = m->nC;

  // copy M into LD: Legacy format
  vector<mjtNum> LDlegacy(nM, 0);
  for (int i=0; i < nC; i++) {
    LDlegacy[d->mapM2C[i]] = d->qLD[i];
  }

  // compare LD and LDs densified matrices
  vector<mjtNum> LDdense(nv*nv);
  mju_sparse2dense(LDdense.data(), d->qLD, nv, nv,
                   d->C_rownnz, d->C_rowadr, d->C_colind);
  vector<mjtNum> LDdense2(nv*nv);
  mj_fullM(m, LDdense2.data(), LDlegacy.data());

  // expect lower triangles to match exactly
  for (int i=0; i < nv; i++) {
    for (int j=0; j < i; j++) {
      EXPECT_EQ(LDdense[i*nv+j], LDdense2[i*nv+j]);
    }
  }

  // compare legacy and CSR LD vector solve
  vector<mjtNum> vec(nv);
  vector<mjtNum> vec2(nv);
  for (int i=0; i < nv; i++) vec[i] = vec2[i] = 20 + 30*i;
  for (int i=0; i < nv; i+=2) vec[i] = vec2[i] = 0;

  mj_solveLD(m, vec.data(), 1, LDlegacy.data(), d->qLDiagInv);
  mj_solveLDs(vec2.data(), d->qLD, d->qLDiagInv, nv, 1,
              d->C_rownnz, d->C_rowadr, m->dof_simplenum, d->C_colind);

  // expect vectors to match up to floating point precision
  for (int i=0; i < nv; i++) {
    EXPECT_FLOAT_EQ(vec[i], vec2[i]);
  }

  mj_deleteData(d);
  mj_deleteModel(m);
}

TEST_F(CoreSmoothTest, SolveLDmultipleVectors) {
  const std::string xml_path = GetTestDataFilePath(kInertiaPath);
  char error[1024];
  mjModel* m = mj_loadXML(xml_path.c_str(), nullptr, error, sizeof(error));
  ASSERT_THAT(m, NotNull()) << "Failed to load model: " << error;

  mjData* d = mj_makeData(m);
  mj_forward(m, d);

  int nv = m->nv;
  int nM = m->nM;
  int nC = m->nC;

  // copy LD into LDlegacy: Legacy format
  vector<mjtNum> LDlegacy(nM, 0);
  for (int i=0; i < nC; i++) {
    LDlegacy[d->mapM2C[i]] = d->qLD[i];
  }

  // compare n LD and LDs vector solve
  int n = 3;
  vector<mjtNum> vec(nv*n);
  vector<mjtNum> vec2(nv*n);
  for (int i=0; i < nv*n; i++) vec[i] = vec2[i] = 2 + 3*i;
  for (int i=0; i < nv*n; i+=3) vec[i] = vec2[i] = 0;

  mj_solveLD(m, vec.data(), n, LDlegacy.data(), d->qLDiagInv);
  mj_solveLDs(vec2.data(), d->qLD, d->qLDiagInv, nv, n,
              d->C_rownnz, d->C_rowadr, m->dof_simplenum, d->C_colind);

  // expect vectors to match up to floating point precision
  for (int i=0; i < nv*n; i++) {
    EXPECT_FLOAT_EQ(vec[i], vec2[i]);
  }

  mj_deleteData(d);
  mj_deleteModel(m);
}

TEST_F(CoreSmoothTest, SolveM2) {
  const std::string xml_path = GetTestDataFilePath(kInertiaPath);
  char error[1024];
  mjModel* m = mj_loadXML(xml_path.c_str(), nullptr, error, sizeof(error));
  ASSERT_THAT(m, NotNull()) << "Failed to load model: " << error;

  mjData* d = mj_makeData(m);
  mj_forward(m, d);

  // inverse square root of D from inertia LDL decomposition
  int nv = m->nv;
  vector<mjtNum> sqrtInvD(nv);
  for (int i=0; i < nv; i++) {
    int diag = d->C_rowadr[i] + d->C_rownnz[i] - 1;
    sqrtInvD[i] = 1 / mju_sqrt(d->qLD[diag]);
  }

  // compare full solve and half solve
  int n = 3;
  vector<mjtNum> vec(nv*n);
  vector<mjtNum> vec2(nv*n);
  for (int i=0; i < nv*n; i++) vec[i] = vec2[i] = 2 + 3*i;
  for (int i=0; i < nv*n; i+=3) vec[i] = vec2[i] = 0;
  vector<mjtNum> res(nv*n);

  mj_solveM2(m, d, res.data(), vec.data(), sqrtInvD.data(), n);
  mj_solveLDs(vec2.data(), d->qLD, d->qLDiagInv, nv, n,
              d->C_rownnz, d->C_rowadr, m->dof_simplenum, d->C_colind);

  // expect equality of dot(v, M^-1 * v) and dot(M^-1/2 * v, M^-1/2 * v)
  for (int i=0; i < n; i++) {
    EXPECT_FLOAT_EQ(mju_dot(vec2.data() + i*nv, vec.data() + i*nv, nv),
                    mju_dot(res.data() + i*nv, res.data() + i*nv, nv));
  }

  mj_deleteData(d);
  mj_deleteModel(m);
}

TEST_F(CoreSmoothTest, FactorIs) {
  const std::string xml_path = GetTestDataFilePath(kInertiaPath);
  char error[1024];
  mjModel* m = mj_loadXML(xml_path.c_str(), nullptr, error, sizeof(error));
  ASSERT_THAT(m, NotNull()) << "Failed to load model: " << error;

  mjData* d = mj_makeData(m);
  mj_forward(m, d);

  int nC = m->nC, nM = m->nM, nv = m->nv;

  // copy qM into into qLDlegacy and factorize
  vector<mjtNum> qLDlegacy(nM);
  mj_factorI(m, d, d->qM, qLDlegacy.data(), d->qLDiagInv);

  // copy qLDlegacy into qLDexpected: CSR format
  vector<mjtNum> qLDexpected(nC);
  for (int i=0; i < nC; i++) {
    qLDexpected[i] = qLDlegacy[d->mapM2C[i]];  // mj_factorIs is in-place
  }

  // copy qM into qLD: CSR format
  vector<mjtNum> qLD(nC);
  for (int i=0; i < nC; i++) {
    qLD[i] = d->qM[d->mapM2C[i]];  // mj_factorIs is in-place
  }

  vector<mjtNum> qLDiagInvExpected(d->qLDiagInv, d->qLDiagInv + nv);
  vector<mjtNum> qLDiagInv(nv, 0);

  mj_factorIs(qLD.data(), qLDiagInv.data(), nv,
              d->C_rownnz, d->C_rowadr, m->dof_simplenum, d->C_colind);

  // expect outputs to match to floating point precision
  EXPECT_THAT(qLD, Pointwise(DoubleNear(1e-12), qLDexpected));
  EXPECT_THAT(qLDiagInv, Pointwise(DoubleNear(1e-12), qLDiagInvExpected));

  /* uncomment for debugging
  vector<mjtNum> LDdense(nv*nv);

  mju_sparse2dense(LDdense.data(), qLDexpected.data(), nv, nv,
                   d->C_rownnz, d->C_rowadr, d->C_colind);
  PrintMatrix(LDdense.data(), nv, nv, 2);

  mju_sparse2dense(LDdense.data(), qLDs.data(), nv, nv,
                   d->C_rownnz, d->C_rowadr, d->C_colind);
  PrintMatrix(LDdense.data(), nv, nv, 2);
  */

  mj_deleteData(d);
  mj_deleteModel(m);
}

}  // namespace
}  // namespace mujoco
