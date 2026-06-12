#ifdef _DEBUG
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Controller/ReleaseController.h"
#include "Model/ISampleRepository.h"
#include "Model/IOrderRepository.h"

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SaveArg;
using ::testing::NiceMock;

namespace {

class FakeClockR : public IClock {
public:
    explicit FakeClockR(std::string t) : time_(std::move(t)) {}
    std::string now() const override { return time_; }
private:
    std::string time_;
};

class MockOrderRepoRC : public IOrderRepository {
public:
    MOCK_METHOD(void,                 add,            (const Order&),               (override));
    MOCK_METHOD(std::optional<Order>, findById,       (const std::string&), (const, override));
    MOCK_METHOD(std::vector<Order>,   findAll,        (),                   (const, override));
    MOCK_METHOD(std::vector<Order>,   findByStatus,   (OrderStatus),        (const, override));
    MOCK_METHOD(std::vector<Order>,   findBySampleId, (const std::string&), (const, override));
    MOCK_METHOD(bool,                 update,         (const Order&),               (override));
    MOCK_METHOD(int,                  count,          (),                   (const, override));
};

class MockSampleRepoRC : public ISampleRepository {
public:
    MOCK_METHOD(void,                  add,        (const Sample&),              (override));
    MOCK_METHOD(std::optional<Sample>, findById,   (const std::string&), (const, override));
    MOCK_METHOD(std::vector<Sample>,   findAll,    (),                   (const, override));
    MOCK_METHOD(std::vector<Sample>,   findByName, (const std::string&), (const, override));
    MOCK_METHOD(bool,                  update,     (const Sample&),              (override));
    MOCK_METHOD(bool,                  remove,     (const std::string&),         (override));
    MOCK_METHOD(int,                   count,      (),                   (const, override));
};

static Sample makeSampleR(const std::string& id, int pureQty,
                          int reservedQty = 0) {
    Sample s{};
    s.id               = id;
    s.name             = "테스트시료";
    s.pureQuantity     = pureQty;
    s.reservedQuantity = reservedQty;
    s.yield            = 0.9;
    s.cycleTime        = 0.5;
    s.registeredAt     = "2026-06-12 09:00:00";
    return s;
}

static Order makeOrderR(const std::string& id, const std::string& sampleId,
                        int qty, OrderStatus status = OrderStatus::CONFIRMED) {
    Order o{};
    o.id                 = id;
    o.sampleId           = sampleId;
    o.customerName       = "테스트고객";
    o.quantity           = qty;
    o.requiredProduction = 0;
    o.status             = status;
    o.orderedAt          = "2026-06-12 09:00:00";
    return o;
}

} // namespace

// ========================================================
// ReleaseController 테스트 픽스처
// ========================================================
class ReleaseControllerTest : public ::testing::Test {
protected:
    NiceMock<MockOrderRepoRC>  orderRepo;
    NiceMock<MockSampleRepoRC> sampleRepo;
    FakeClockR                 clock{"2026-06-12 09:00:00"};
    ReleaseController          ctrl{orderRepo, sampleRepo, clock};
};

// T6-10: getConfirmedOrders → findByStatus(CONFIRMED) 위임
TEST_F(ReleaseControllerTest, GetConfirmedOrders_OnlyConfirmed) {
    std::vector<Order> confirmed = {
        makeOrderR("ORD0001", "S-001", 150, OrderStatus::CONFIRMED),
        makeOrderR("ORD0002", "S-002", 400, OrderStatus::CONFIRMED)
    };
    EXPECT_CALL(orderRepo, findByStatus(OrderStatus::CONFIRMED))
        .WillOnce(Return(confirmed));

    auto result = ctrl.getConfirmedOrders();
    EXPECT_EQ(result.size(), 2u);
}

// T6-11: releaseOrder 성공 → RELEASE 상태
TEST_F(ReleaseControllerTest, ReleaseOrder_Success) {
    Order  o = makeOrderR("ORD0001", "S-001", 150, OrderStatus::CONFIRMED);
    Sample s = makeSampleR("S-001", 0, 200);
    EXPECT_CALL(orderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, findById("S-001"))  .WillOnce(Return(s));
    EXPECT_CALL(sampleRepo, update(_)).WillOnce(Return(true));

    Order capturedOrder{};
    EXPECT_CALL(orderRepo, update(_))
        .WillOnce(DoAll(SaveArg<0>(&capturedOrder), Return(true)));

    EXPECT_TRUE(ctrl.releaseOrder("ORD0001"));
    EXPECT_EQ(capturedOrder.status, OrderStatus::RELEASE);
}

// T6-12: releaseOrder — reservedQty 감소 확인 (200 - 150 = 50)
TEST_F(ReleaseControllerTest, ReleaseOrder_ReservedQtyDecreases) {
    Order  o = makeOrderR("ORD0001", "S-001", 150, OrderStatus::CONFIRMED);
    Sample s = makeSampleR("S-001", 0, 200);
    EXPECT_CALL(orderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, findById("S-001"))  .WillOnce(Return(s));
    EXPECT_CALL(orderRepo,  update(_)).WillOnce(Return(true));

    Sample capturedSample{};
    EXPECT_CALL(sampleRepo, update(_))
        .WillOnce(DoAll(SaveArg<0>(&capturedSample), Return(true)));

    ctrl.releaseOrder("ORD0001");
    EXPECT_EQ(capturedSample.reservedQuantity, 50);
}

// T6-13: releaseOrder — pureQty 불변
TEST_F(ReleaseControllerTest, ReleaseOrder_PureQtyUnchanged) {
    Order  o = makeOrderR("ORD0001", "S-001", 150, OrderStatus::CONFIRMED);
    Sample s = makeSampleR("S-001", 30, 200);
    EXPECT_CALL(orderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, findById("S-001"))  .WillOnce(Return(s));
    EXPECT_CALL(orderRepo,  update(_)).WillOnce(Return(true));

    Sample capturedSample{};
    EXPECT_CALL(sampleRepo, update(_))
        .WillOnce(DoAll(SaveArg<0>(&capturedSample), Return(true)));

    ctrl.releaseOrder("ORD0001");
    EXPECT_EQ(capturedSample.pureQuantity, 30); // 불변
}

// T6-14: CONFIRMED 아닌 주문 출고 시도 → false
TEST_F(ReleaseControllerTest, ReleaseOrder_NotConfirmed) {
    Order o = makeOrderR("ORD0001", "S-001", 150, OrderStatus::PRODUCING);
    EXPECT_CALL(orderRepo,  findById("ORD0001")).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, update(_)).Times(0);
    EXPECT_CALL(orderRepo,  update(_)).Times(0);

    EXPECT_FALSE(ctrl.releaseOrder("ORD0001"));
}

// T6-15: 없는 orderId → false
TEST_F(ReleaseControllerTest, ReleaseOrder_NotFound) {
    EXPECT_CALL(orderRepo, findById("ORD9999")).WillOnce(Return(std::nullopt));

    EXPECT_FALSE(ctrl.releaseOrder("ORD9999"));
}

#endif
