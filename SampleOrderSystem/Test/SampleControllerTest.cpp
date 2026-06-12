#ifdef _DEBUG
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Controller/SampleController.h"
#include "Model/ISampleRepository.h"

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;

// ---- FakeClock ----
class FakeClockS : public IClock {
public:
    explicit FakeClockS(std::string t) : time_(std::move(t)) {}
    std::string now() const override { return time_; }
private:
    std::string time_;
};

// ---- MockSampleRepository ----
class MockSampleRepo : public ISampleRepository {
public:
    MOCK_METHOD(void,                  add,        (const Sample&),              (override));
    MOCK_METHOD(std::optional<Sample>, findById,   (const std::string&), (const, override));
    MOCK_METHOD(std::vector<Sample>,   findAll,    (),                   (const, override));
    MOCK_METHOD(std::vector<Sample>,   findByName, (const std::string&), (const, override));
    MOCK_METHOD(bool,                  update,     (const Sample&),              (override));
    MOCK_METHOD(bool,                  remove,     (const std::string&),         (override));
    MOCK_METHOD(int,                   count,      (),                   (const, override));
};

// ---- 테스트 헬퍼 ----
static Sample makeSampleForCtrl(const std::string& id, int pureQty, int reservedQty) {
    Sample s{};
    s.id               = id;
    s.pureQuantity     = pureQty;
    s.reservedQuantity = reservedQty;
    return s;
}

// ========================================================
// SampleController 테스트
// ========================================================
class SampleControllerTest : public ::testing::Test {
protected:
    NiceMock<MockSampleRepo> repo;
    FakeClockS               clock{"2026-06-12 09:00:00"};
    SampleController         ctrl{repo, clock};
};

// T4-1: 신규 ID → 등록 성공, repo.add 1회 호출
TEST_F(SampleControllerTest, AddSample_Success) {
    EXPECT_CALL(repo, findById("SAM1")).WillOnce(Return(std::nullopt));
    EXPECT_CALL(repo, add(_)).Times(1);

    EXPECT_TRUE(ctrl.addSample("SAM1", "실리콘 웨이퍼", 500, 0.92, 0.5));
}

// T4-2: 중복 ID → 등록 실패, repo.add 미호출
TEST_F(SampleControllerTest, AddSample_DuplicateId) {
    Sample existing{};
    existing.id = "SAM1";
    EXPECT_CALL(repo, findById("SAM1")).WillOnce(Return(existing));
    EXPECT_CALL(repo, add(_)).Times(0);

    EXPECT_FALSE(ctrl.addSample("SAM1", "중복", 100, 0.9, 0.5));
}

// T4-3: registeredAt = FakeClock 값
TEST_F(SampleControllerTest, AddSample_SetsRegisteredAt) {
    EXPECT_CALL(repo, findById(_)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(repo, add(::testing::Field(&Sample::registeredAt,
                                           "2026-06-12 09:00:00"))).Times(1);
    ctrl.addSample("SAM1", "테스트", 100, 0.9, 0.5);
}

// T4-4: reservedQuantity 초기값 0
TEST_F(SampleControllerTest, AddSample_ReservedQuantityZero) {
    EXPECT_CALL(repo, findById(_)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(repo, add(::testing::Field(&Sample::reservedQuantity, 0))).Times(1);
    ctrl.addSample("SAM1", "테스트", 100, 0.9, 0.5);
}

// T4-5: getAllSamples → repo.findAll 위임
TEST_F(SampleControllerTest, GetAllSamples_DelegatesToRepo) {
    EXPECT_CALL(repo, findAll()).WillOnce(Return(std::vector<Sample>{}));
    ctrl.getAllSamples();
}

// T4-6: searchByName → repo.findByName 위임
TEST_F(SampleControllerTest, SearchByName_DelegatesToRepo) {
    EXPECT_CALL(repo, findByName("웨이퍼")).WillOnce(Return(std::vector<Sample>{}));
    ctrl.searchByName("웨이퍼");
}

// T4-7: getSampleCount → repo.count 위임
TEST_F(SampleControllerTest, GetSampleCount_DelegatesToRepo) {
    EXPECT_CALL(repo, count()).WillOnce(Return(3));
    EXPECT_EQ(ctrl.getSampleCount(), 3);
}

// T4-8: getTotalInventory — pureQty 만 합산 (reservedQty 는 미확보 생산분 포함 가능)
TEST_F(SampleControllerTest, GetTotalInventory_SumsPureQtyOnly) {
    Sample s1 = makeSampleForCtrl("SAM1", 100, 20);
    Sample s2 = makeSampleForCtrl("SAM2", 200, 30);
    EXPECT_CALL(repo, findAll()).WillOnce(Return(std::vector<Sample>{s1, s2}));
    EXPECT_EQ(ctrl.getTotalInventory(), 300); // 100 + 200 (reservedQty 제외)
}

// T4-9: getTotalInventory — 시료 없음 → 0
TEST_F(SampleControllerTest, GetTotalInventory_EmptyRepo) {
    EXPECT_CALL(repo, findAll()).WillOnce(Return(std::vector<Sample>{}));
    EXPECT_EQ(ctrl.getTotalInventory(), 0);
}
#endif
