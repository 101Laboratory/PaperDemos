#include "pch.h"

#include "MathUtils.h"

using namespace DirectX;

TEST(MathUtils, Coordinates) {
  {
    auto [r, phi, theta] = ToSpherical(1, 0, 0);
    EXPECT_NEAR(r, 1.0f, 1e-6);
    EXPECT_NEAR(phi, 0.0f, 1e-6);
    EXPECT_NEAR(theta, XM_PIDIV2, 1e-6);
  }
  {
    auto [r, phi, theta] = ToSpherical(1, 0, 1);
    EXPECT_NEAR(r, std::sqrt(2), 1e-6);
    EXPECT_NEAR(phi, XM_PIDIV4, 1e-6);
    EXPECT_NEAR(theta, XM_PIDIV2, 1e-6);
  }
  {
    auto [r, phi, theta] = ToSpherical(1, 0, -1);
    EXPECT_NEAR(phi, -XM_PIDIV4, 1e-6);
    EXPECT_NEAR(theta, XM_PIDIV2, 1e-6);
  }
  {
    auto [r, phi, theta] = ToSpherical(0, 0, -1);
    EXPECT_NEAR(phi, -XM_PIDIV2, 1e-6);
    EXPECT_NEAR(theta, XM_PIDIV2, 1e-6);
  }
  { auto [x, y, z] = ToCartesian(1.0f, XM_PIDIV4, XM_PIDIV4);
    EXPECT_NEAR(x, 0.5, 1e-6);
    EXPECT_NEAR(y, sqrt(2) / 2.0f, 1e-6);
    EXPECT_NEAR(z, 0.5, 1e-6);
  }
  {
    auto [x, y, z] = ToCartesian(5.0f, XM_PIDIV2, XM_PIDIV2);
    EXPECT_NEAR(x, 0, 1e-6);
    EXPECT_NEAR(y, 0, 1e-6);
    EXPECT_NEAR(z, 5.0f, 1e-6);
  }
  {
    auto [x, y, z] = ToCartesian(5.0f, -XM_PIDIV2, XM_PIDIV2);
    EXPECT_NEAR(x, 0, 1e-6);
    EXPECT_NEAR(y, 0, 1e-6);
    EXPECT_NEAR(z, -5.0f, 1e-6);
  }
}