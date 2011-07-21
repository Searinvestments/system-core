// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstring>

#include <base/file_util.h>
#include <gtest/gtest.h>

#include "c_metrics_library.h"
#include "metrics_library.h"

static const FilePath kTestUMAEventsFile("test-uma-events");
static const char kTestConsent[] = "test-consent";
static const char kTestMounts[] = "test-mounts";

static void SetMetricsEnabled(bool enabled) {
  if (enabled)
    ASSERT_EQ(1, file_util::WriteFile(FilePath(kTestConsent) , "0", 1));
  else
    file_util::Delete(FilePath(kTestConsent), false);
}

class MetricsLibraryTest : public testing::Test {
 protected:
  virtual void SetUp() {
    EXPECT_EQ(NULL, lib_.uma_events_file_);
    lib_.Init();
    EXPECT_TRUE(NULL != lib_.uma_events_file_);
    lib_.uma_events_file_ = kTestUMAEventsFile.value().c_str();
    SetMetricsEnabled(true);
    // Defeat metrics enabled caching between tests.
    lib_.cached_enabled_time_ = 0;
    lib_.consent_file_ = kTestConsent;
  }

  virtual void TearDown() {
    file_util::Delete(FilePath(kTestConsent), false);
    file_util::Delete(FilePath(kTestMounts), false);
    file_util::Delete(kTestUMAEventsFile, false);
  }

  void VerifyEnabledCacheHit(bool to_value);
  void VerifyEnabledCacheEviction(bool to_value);

  MetricsLibrary lib_;
};

TEST_F(MetricsLibraryTest, IsDeviceMounted) {
  static const char kTestContents[] =
      "0123456789abcde 0123456789abcde\nguestfs foo bar\n";
  char buffer[1024];
  int block_sizes[] = { 1, 2, 3, 4, 5, 6, 8, 12, 14, 16, 32, 1024 };
  bool result;
  EXPECT_FALSE(lib_.IsDeviceMounted("guestfs",
                                    "nonexistent",
                                    buffer,
                                    1,
                                    &result));
  ASSERT_TRUE(file_util::WriteFile(FilePath(kTestMounts),
                                   kTestContents,
                                   strlen(kTestContents)));
  EXPECT_FALSE(lib_.IsDeviceMounted("guestfs",
                                    kTestMounts,
                                    buffer,
                                    0,
                                    &result));
  for (size_t i = 0; i < arraysize(block_sizes); ++i) {
    EXPECT_TRUE(lib_.IsDeviceMounted("0123456789abcde",
                                     kTestMounts,
                                     buffer,
                                     block_sizes[i],
                                     &result));
    EXPECT_TRUE(result);
    EXPECT_TRUE(lib_.IsDeviceMounted("guestfs",
                                     kTestMounts,
                                     buffer,
                                     block_sizes[i],
                                     &result));
    EXPECT_TRUE(result);
    EXPECT_TRUE(lib_.IsDeviceMounted("0123456",
                                     kTestMounts,
                                     buffer,
                                     block_sizes[i],
                                     &result));
    EXPECT_FALSE(result);
    EXPECT_TRUE(lib_.IsDeviceMounted("9abcde",
                                     kTestMounts,
                                     buffer,
                                     block_sizes[i],
                                     &result));
    EXPECT_FALSE(result);
    EXPECT_TRUE(lib_.IsDeviceMounted("foo",
                                     kTestMounts,
                                     buffer,
                                     block_sizes[i],
                                     &result));
    EXPECT_FALSE(result);
    EXPECT_TRUE(lib_.IsDeviceMounted("bar",
                                     kTestMounts,
                                     buffer,
                                     block_sizes[i],
                                     &result));
    EXPECT_FALSE(result);
  }
}

TEST_F(MetricsLibraryTest, AreMetricsEnabledFalse) {
  SetMetricsEnabled(false);
  EXPECT_FALSE(lib_.AreMetricsEnabled());
}

TEST_F(MetricsLibraryTest, AreMetricsEnabledTrue) {
  EXPECT_TRUE(lib_.AreMetricsEnabled());
}

void MetricsLibraryTest::VerifyEnabledCacheHit(bool to_value) {
  // We might step from one second to the next one time, but not 100
  // times in a row.
  for (int i = 0; i < 100; ++i) {
    lib_.cached_enabled_time_ = 0;
    SetMetricsEnabled(!to_value);
    ASSERT_EQ(!to_value, lib_.AreMetricsEnabled());
    SetMetricsEnabled(to_value);
    if (lib_.AreMetricsEnabled() == !to_value)
      return;
  }
  ADD_FAILURE() << "Did not see evidence of caching";
}

void MetricsLibraryTest::VerifyEnabledCacheEviction(bool to_value) {
  lib_.cached_enabled_time_ = 0;
  SetMetricsEnabled(!to_value);
  ASSERT_EQ(!to_value, lib_.AreMetricsEnabled());
  SetMetricsEnabled(to_value);
  ASSERT_LT(abs(time(NULL) - lib_.cached_enabled_time_), 5);
  // Sleep one second (or cheat to be faster).
  --lib_.cached_enabled_time_;
  ASSERT_EQ(to_value, lib_.AreMetricsEnabled());
}

TEST_F(MetricsLibraryTest, AreMetricsEnabledCaching) {
  VerifyEnabledCacheHit(false);
  VerifyEnabledCacheHit(true);
  VerifyEnabledCacheEviction(false);
  VerifyEnabledCacheEviction(true);
}

TEST_F(MetricsLibraryTest, FormatChromeMessage) {
  char buf[7];
  const int kLen = 6;
  EXPECT_EQ(kLen, lib_.FormatChromeMessage(7, buf, "%d", 1));

  char exp[kLen];
  sprintf(exp, "%c%c%c%c1", kLen, 0, 0, 0);
  EXPECT_EQ(0, memcmp(exp, buf, kLen));
}

TEST_F(MetricsLibraryTest, FormatChromeMessageTooLong) {
  char buf[7];
  EXPECT_EQ(-1, lib_.FormatChromeMessage(7, buf, "test"));
}

TEST_F(MetricsLibraryTest, SendEnumToUMA) {
  char buf[100];
  const int kLen = 40;
  EXPECT_TRUE(lib_.SendEnumToUMA("Test.EnumMetric", 1, 3));
  EXPECT_EQ(kLen, file_util::ReadFile(kTestUMAEventsFile, buf, 100));

  char exp[kLen];
  sprintf(exp, "%c%c%c%clinearhistogram%cTest.EnumMetric 1 3",
          kLen, 0, 0, 0, 0);
  EXPECT_EQ(0, memcmp(exp, buf, kLen));
}

TEST_F(MetricsLibraryTest, SendEnumToUMANotEnabled) {
  SetMetricsEnabled(false);
  EXPECT_TRUE(lib_.SendEnumToUMA("Test.EnumMetric", 1, 3));
  EXPECT_FALSE(file_util::PathExists(kTestUMAEventsFile));
}

TEST_F(MetricsLibraryTest, SendMessageToChrome) {
  EXPECT_TRUE(lib_.SendMessageToChrome(4, "test"));
  EXPECT_TRUE(lib_.SendMessageToChrome(7, "content"));
  std::string uma_events;
  EXPECT_TRUE(file_util::ReadFileToString(kTestUMAEventsFile, &uma_events));
  EXPECT_EQ("testcontent", uma_events);
}

TEST_F(MetricsLibraryTest, SendMessageToChromeUMAEventsBadFileLocation) {
  // Checks that the library doesn't die badly if the file can't be
  // created.
  static const char kDoesNotExistFile[] = "/does/not/exist";
  lib_.uma_events_file_ = kDoesNotExistFile;
  static const char kDummyMessage[] = "Dummy Message";
  EXPECT_FALSE(lib_.SendMessageToChrome(strlen(kDummyMessage), kDummyMessage));
  file_util::Delete(FilePath(kDoesNotExistFile), false);
}

TEST_F(MetricsLibraryTest, SendToUMA) {
  char buf[100];
  const int kLen = 37;
  EXPECT_TRUE(lib_.SendToUMA("Test.Metric", 2, 1, 100, 50));
  EXPECT_EQ(kLen, file_util::ReadFile(kTestUMAEventsFile, buf, 100));

  char exp[kLen];
  sprintf(exp, "%c%c%c%chistogram%cTest.Metric 2 1 100 50", kLen, 0, 0, 0, 0);
  EXPECT_EQ(0, memcmp(exp, buf, kLen));
}

TEST_F(MetricsLibraryTest, SendToUMANotEnabled) {
  SetMetricsEnabled(false);
  EXPECT_TRUE(lib_.SendToUMA("Test.Metric", 2, 1, 100, 50));
  EXPECT_FALSE(file_util::PathExists(kTestUMAEventsFile));
}

TEST_F(MetricsLibraryTest, SendUserActionToUMA) {
  char buf[100];
  const int kLen = 30;
  EXPECT_TRUE(lib_.SendUserActionToUMA("SomeKeyPressed"));
  EXPECT_EQ(kLen, file_util::ReadFile(kTestUMAEventsFile, buf, 100));

  char exp[kLen];
  sprintf(exp, "%c%c%c%cuseraction%cSomeKeyPressed", kLen, 0, 0, 0, 0);
  EXPECT_EQ(0, memcmp(exp, buf, kLen));
}

TEST_F(MetricsLibraryTest, SendUserActionToUMANotEnabled) {
  SetMetricsEnabled(false);
  EXPECT_TRUE(lib_.SendUserActionToUMA("SomeOtherKeyPressed"));
  EXPECT_FALSE(file_util::PathExists(kTestUMAEventsFile));
}

TEST_F(MetricsLibraryTest, SendCrashToUMAEnabled) {
  EXPECT_TRUE(lib_.SendCrashToUMA("kernel"));
  char exp[100];
  int len = sprintf(exp, "%c%c%c%ccrash%ckernel",
                    0, 0, 0, 0, 0) + 1;
  exp[0] = len;
  char buf[100];
  EXPECT_EQ(len, file_util::ReadFile(kTestUMAEventsFile, buf, 100));
  EXPECT_EQ(0, memcmp(exp, buf, len));
}

TEST_F(MetricsLibraryTest, SendCrashToUMANotEnabled) {
  SetMetricsEnabled(false);
  EXPECT_TRUE(lib_.SendCrashToUMA("kernel"));
  EXPECT_FALSE(file_util::PathExists(kTestUMAEventsFile));
}

class CMetricsLibraryTest : public testing::Test {
 protected:
  virtual void SetUp() {
    lib_ = CMetricsLibraryNew();
    MetricsLibrary& ml = *reinterpret_cast<MetricsLibrary*>(lib_);
    EXPECT_EQ(NULL, ml.uma_events_file_);
    CMetricsLibraryInit(lib_);
    EXPECT_TRUE(NULL != ml.uma_events_file_);
    ml.uma_events_file_ = kTestUMAEventsFile.value().c_str();
    SetMetricsEnabled(true);
    reinterpret_cast<MetricsLibrary*>(lib_)->cached_enabled_time_ = 0;
    reinterpret_cast<MetricsLibrary*>(lib_)->consent_file_ = kTestConsent;
  }

  virtual void TearDown() {
    CMetricsLibraryDelete(lib_);
    file_util::Delete(kTestUMAEventsFile, false);
  }

  CMetricsLibrary lib_;
};

TEST_F(CMetricsLibraryTest, AreMetricsEnabledFalse) {
  SetMetricsEnabled(false);
  EXPECT_FALSE(CMetricsLibraryAreMetricsEnabled(lib_));
}

TEST_F(CMetricsLibraryTest, AreMetricsEnabledTrue) {
  EXPECT_TRUE(CMetricsLibraryAreMetricsEnabled(lib_));
}

TEST_F(CMetricsLibraryTest, SendEnumToUMA) {
  char buf[100];
  const int kLen = 40;
  EXPECT_TRUE(CMetricsLibrarySendEnumToUMA(lib_, "Test.EnumMetric", 1, 3));
  EXPECT_EQ(kLen, file_util::ReadFile(kTestUMAEventsFile, buf, 100));

  char exp[kLen];
  sprintf(exp, "%c%c%c%clinearhistogram%cTest.EnumMetric 1 3",
          kLen, 0, 0, 0, 0);
  EXPECT_EQ(0, memcmp(exp, buf, kLen));
}

TEST_F(CMetricsLibraryTest, SendToUMA) {
  char buf[100];
  const int kLen = 37;
  EXPECT_TRUE(CMetricsLibrarySendToUMA(lib_, "Test.Metric", 2, 1, 100, 50));
  EXPECT_EQ(kLen, file_util::ReadFile(kTestUMAEventsFile, buf, 100));

  char exp[kLen];
  sprintf(exp, "%c%c%c%chistogram%cTest.Metric 2 1 100 50", kLen, 0, 0, 0, 0);
  EXPECT_EQ(0, memcmp(exp, buf, kLen));
}

TEST_F(CMetricsLibraryTest, SendUserActionToUMA) {
  char buf[100];
  const int kLen = 30;
  EXPECT_TRUE(CMetricsLibrarySendUserActionToUMA(lib_, "SomeKeyPressed"));
  EXPECT_EQ(kLen, file_util::ReadFile(kTestUMAEventsFile, buf, 100));

  char exp[kLen];
  sprintf(exp, "%c%c%c%cuseraction%cSomeKeyPressed", kLen, 0, 0, 0, 0);
  EXPECT_EQ(0, memcmp(exp, buf, kLen));
}

TEST_F(CMetricsLibraryTest, SendCrashToUMA) {
  char buf[100];
  char exp[100];
  int len = sprintf(exp, "%c%c%c%ccrash%cuser", 0, 0, 0, 0, 0) + 1;
  exp[0] = len;
  EXPECT_TRUE(CMetricsLibrarySendCrashToUMA(lib_, "user"));
  EXPECT_EQ(len, file_util::ReadFile(kTestUMAEventsFile, buf, 100));

  EXPECT_EQ(0, memcmp(exp, buf, len));
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
