#ifdef _DEBUG
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Model/ISampleRepository.h"
#include "Model/IOrderRepository.h"
#include "Model/SampleRepository.h"
#include "Model/OrderRepository.h"

// ========================================================
// Mock 클래스 — Phase 4~에서 Controller 단위 테스트에 재사용
// ========================================================
class MockSampleRepository : public ISampleRepository {
public:
    MOCK_METHOD(void,                  add,        (const Sample&),              (override));
    MOCK_METHOD(std::optional<Sample>, findById,   (const std::string&), (const, override));
    MOCK_METHOD(std::vector<Sample>,   findAll,    (),                   (const, override));
    MOCK_METHOD(std::vector<Sample>,   findByName, (const std::string&), (const, override));
    MOCK_METHOD(bool,                  update,     (const Sample&),              (override));
    MOCK_METHOD(bool,                  remove,     (const std::string&),         (override));
    MOCK_METHOD(int,                   count,      (),                   (const, override));
};

class MockOrderRepository : public IOrderRepository {
public:
    MOCK_METHOD(void,                 add,            (const Order&),               (override));
    MOCK_METHOD(std::optional<Order>, findById,       (const std::string&), (const, override));
    MOCK_METHOD(std::vector<Order>,   findAll,        (),                   (const, override));
    MOCK_METHOD(std::vector<Order>,   findByStatus,   (OrderStatus),        (const, override));
    MOCK_METHOD(std::vector<Order>,   findBySampleId, (const std::string&), (const, override));
    MOCK_METHOD(bool,                 update,         (const Order&),               (override));
    MOCK_METHOD(int,                  count,          (),                   (const, override));
};

// ========================================================
// 테스트 헬퍼
// ========================================================
static Sample makeSample(const std::string& id, const std::string& name,
                         int pureQty = 100, int reservedQty = 0,
                         double yield = 0.9, double cycleTime = 0.5) {
    Sample s;
    s.id               = id;
    s.name             = name;
    s.pureQuantity     = pureQty;
    s.reservedQuantity = reservedQty;
    s.yield            = yield;
    s.cycleTime        = cycleTime;
    s.registeredAt     = "2026-06-12 09:00:00";
    return s;
}

static Order makeOrder(const std::string& id, const std::string& sampleId,
                       int qty = 50, OrderStatus status = OrderStatus::RESERVED) {
    Order o{};
    o.id               = id;
    o.sampleId         = sampleId;
    o.customerName     = "테스트고객";
    o.quantity         = qty;
    o.status           = status;
    o.orderedAt        = "2026-06-12 09:00:00";
    o.requiredProduction = 0;
    return o;
}

// ========================================================
// SampleRepository 테스트
// ========================================================
class SampleRepositoryTest : public ::testing::Test {
protected:
    SampleRepository repo;
};

TEST_F(SampleRepositoryTest, Add_IncreasesCount) {
    repo.add(makeSample("SAM1", "실리콘 웨이퍼"));
    EXPECT_EQ(repo.count(), 1);
}

TEST_F(SampleRepositoryTest, FindById_Exists) {
    repo.add(makeSample("SAM1", "실리콘 웨이퍼", 200));
    auto result = repo.findById("SAM1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id,           "SAM1");
    EXPECT_EQ(result->name,         "실리콘 웨이퍼");
    EXPECT_EQ(result->pureQuantity, 200);
}

TEST_F(SampleRepositoryTest, FindById_NotExists) {
    auto result = repo.findById("NONE");
    EXPECT_FALSE(result.has_value());
}

TEST_F(SampleRepositoryTest, FindAll_ReturnsAllItems) {
    repo.add(makeSample("SAM1", "A"));
    repo.add(makeSample("SAM2", "B"));
    repo.add(makeSample("SAM3", "C"));
    EXPECT_EQ(repo.findAll().size(), 3u);
}

TEST_F(SampleRepositoryTest, FindAll_Empty) {
    EXPECT_TRUE(repo.findAll().empty());
}

TEST_F(SampleRepositoryTest, FindByName_PartialMatch) {
    repo.add(makeSample("SAM1", "실리콘 웨이퍼-8인치"));
    repo.add(makeSample("SAM2", "GaN 웨이퍼-4인치"));
    repo.add(makeSample("SAM3", "SiC 파워기판"));
    auto results = repo.findByName("웨이퍼");
    EXPECT_EQ(results.size(), 2u);
}

TEST_F(SampleRepositoryTest, FindByName_NoMatch) {
    repo.add(makeSample("SAM1", "실리콘 웨이퍼"));
    EXPECT_TRUE(repo.findByName("없는키워드").empty());
}

TEST_F(SampleRepositoryTest, FindByName_ExactMatch) {
    repo.add(makeSample("SAM1", "실리콘 웨이퍼-8인치"));
    repo.add(makeSample("SAM2", "GaN 에피택셜"));
    auto results = repo.findByName("GaN 에피택셜");
    EXPECT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].id, "SAM2");
}

TEST_F(SampleRepositoryTest, Update_Success) {
    repo.add(makeSample("SAM1", "실리콘 웨이퍼", 100));
    Sample updated = makeSample("SAM1", "실리콘 웨이퍼", 999);
    EXPECT_TRUE(repo.update(updated));
    EXPECT_EQ(repo.findById("SAM1")->pureQuantity, 999);
}

TEST_F(SampleRepositoryTest, Update_NotExists) {
    EXPECT_FALSE(repo.update(makeSample("NONE", "없음")));
}

TEST_F(SampleRepositoryTest, Remove_Success) {
    repo.add(makeSample("SAM1", "실리콘 웨이퍼"));
    EXPECT_TRUE(repo.remove("SAM1"));
    EXPECT_FALSE(repo.findById("SAM1").has_value());
}

TEST_F(SampleRepositoryTest, Remove_NotExists) {
    EXPECT_FALSE(repo.remove("NONE"));
}

TEST_F(SampleRepositoryTest, Remove_DecreasesCount) {
    repo.add(makeSample("SAM1", "A"));
    repo.add(makeSample("SAM2", "B"));
    repo.remove("SAM1");
    EXPECT_EQ(repo.count(), 1);
}

// ========================================================
// OrderRepository 테스트
// ========================================================
class OrderRepositoryTest : public ::testing::Test {
protected:
    OrderRepository repo;
};

TEST_F(OrderRepositoryTest, Add_IncreasesCount) {
    repo.add(makeOrder("ORD1", "SAM1"));
    EXPECT_EQ(repo.count(), 1);
}

TEST_F(OrderRepositoryTest, FindById_Exists) {
    repo.add(makeOrder("ORD1", "SAM1", 100));
    auto result = repo.findById("ORD1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id,       "ORD1");
    EXPECT_EQ(result->quantity, 100);
}

TEST_F(OrderRepositoryTest, FindById_NotExists) {
    EXPECT_FALSE(repo.findById("NONE").has_value());
}

TEST_F(OrderRepositoryTest, FindByStatus_Reserved) {
    repo.add(makeOrder("ORD1", "SAM1", 50, OrderStatus::RESERVED));
    repo.add(makeOrder("ORD2", "SAM1", 50, OrderStatus::RESERVED));
    repo.add(makeOrder("ORD3", "SAM1", 50, OrderStatus::CONFIRMED));
    auto results = repo.findByStatus(OrderStatus::RESERVED);
    EXPECT_EQ(results.size(), 2u);
}

TEST_F(OrderRepositoryTest, FindByStatus_Rejected) {
    repo.add(makeOrder("ORD1", "SAM1", 50, OrderStatus::REJECTED));
    auto results = repo.findByStatus(OrderStatus::REJECTED);
    EXPECT_EQ(results.size(), 1u);
}

TEST_F(OrderRepositoryTest, FindByStatus_Empty) {
    repo.add(makeOrder("ORD1", "SAM1", 50, OrderStatus::RESERVED));
    EXPECT_TRUE(repo.findByStatus(OrderStatus::CONFIRMED).empty());
}

TEST_F(OrderRepositoryTest, FindBySampleId_Match) {
    repo.add(makeOrder("ORD1", "SAM1"));
    repo.add(makeOrder("ORD2", "SAM1"));
    repo.add(makeOrder("ORD3", "SAM2"));
    auto results = repo.findBySampleId("SAM1");
    EXPECT_EQ(results.size(), 2u);
}

TEST_F(OrderRepositoryTest, FindBySampleId_NoMatch) {
    repo.add(makeOrder("ORD1", "SAM1"));
    EXPECT_TRUE(repo.findBySampleId("SAM9").empty());
}

TEST_F(OrderRepositoryTest, Update_StatusChange) {
    repo.add(makeOrder("ORD1", "SAM1", 50, OrderStatus::RESERVED));
    Order updated = makeOrder("ORD1", "SAM1", 50, OrderStatus::CONFIRMED);
    EXPECT_TRUE(repo.update(updated));
    EXPECT_EQ(repo.findById("ORD1")->status, OrderStatus::CONFIRMED);
}

TEST_F(OrderRepositoryTest, Update_NotExists) {
    EXPECT_FALSE(repo.update(makeOrder("NONE", "SAM1")));
}

TEST_F(OrderRepositoryTest, FifoOrder_PreservedOnFindAll) {
    repo.add(makeOrder("ORD1", "SAM1"));
    repo.add(makeOrder("ORD2", "SAM1"));
    repo.add(makeOrder("ORD3", "SAM1"));
    auto all = repo.findAll();
    ASSERT_EQ(all.size(), 3u);
    EXPECT_EQ(all[0].id, "ORD1");
    EXPECT_EQ(all[1].id, "ORD2");
    EXPECT_EQ(all[2].id, "ORD3");
}

TEST_F(OrderRepositoryTest, FifoOrder_PreservedOnFindByStatus) {
    repo.add(makeOrder("ORD1", "SAM1", 50, OrderStatus::RESERVED));
    repo.add(makeOrder("ORD2", "SAM1", 50, OrderStatus::RESERVED));
    repo.add(makeOrder("ORD3", "SAM1", 50, OrderStatus::RESERVED));
    auto results = repo.findByStatus(OrderStatus::RESERVED);
    ASSERT_EQ(results.size(), 3u);
    EXPECT_EQ(results[0].id, "ORD1");
    EXPECT_EQ(results[1].id, "ORD2");
    EXPECT_EQ(results[2].id, "ORD3");
}

TEST_F(OrderRepositoryTest, FindAll_Empty) {
    EXPECT_TRUE(repo.findAll().empty());
}

// ========================================================
// Mock 컴파일 검증 — Mock 클래스가 정상 빌드되는지 확인
// ========================================================
TEST(MockRepository, MockSampleRepository_Compiles) {
    MockSampleRepository mock;
    EXPECT_CALL(mock, count()).WillOnce(::testing::Return(42));
    EXPECT_EQ(mock.count(), 42);
}

TEST(MockRepository, MockOrderRepository_Compiles) {
    MockOrderRepository mock;
    EXPECT_CALL(mock, count()).WillOnce(::testing::Return(7));
    EXPECT_EQ(mock.count(), 7);
}
#endif
