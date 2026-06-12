#ifdef _DEBUG
#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include "Model/SampleRepository.h"
#include "Model/OrderRepository.h"
#include "Util/json.hpp"

namespace {

const std::string SAMPLE_PATH = "test_temp_samples.json";
const std::string ORDER_PATH  = "test_temp_orders.json";

static Sample makeSampleP8(const std::string& id, const std::string& name,
                            int pureQty, int reservedQty = 0,
                            double yield = 0.9, double cycleTime = 0.5) {
    Sample s{};
    s.id               = id;
    s.name             = name;
    s.pureQuantity     = pureQty;
    s.reservedQuantity = reservedQty;
    s.yield            = yield;
    s.cycleTime        = cycleTime;
    s.registeredAt     = "2026-06-12 09:00:00";
    return s;
}

static Order makeOrderP8(const std::string& id, const std::string& sampleId,
                         int qty, OrderStatus status,
                         int shortage = 0, const std::string& startedAt = "") {
    Order o{};
    o.id                  = id;
    o.sampleId            = sampleId;
    o.customerName        = "테스트고객";
    o.quantity            = qty;
    o.status              = status;
    o.orderedAt           = "2026-06-12 09:00:00";
    o.requiredProduction  = 0;
    o.productionShortage  = shortage;
    o.productionStartedAt = startedAt;
    return o;
}

} // namespace

class PersistenceTest : public ::testing::Test {
protected:
    void TearDown() override {
        std::remove(SAMPLE_PATH.c_str());
        std::remove(ORDER_PATH.c_str());
    }
};

// T8-1: Sample save/load 왕복
TEST_F(PersistenceTest, SampleRepo_SaveLoad_RoundTrip) {
    {
        SampleRepository repo(SAMPLE_PATH);
        repo.add(makeSampleP8("S-001", "실리콘 웨이퍼", 480, 20, 0.92, 0.5));
        repo.add(makeSampleP8("S-002", "GaN 에피택셜",  220,  0, 0.78, 0.3));
    }
    SampleRepository loaded(SAMPLE_PATH);
    EXPECT_EQ(loaded.count(), 2);
    // ASSERT 전 변수 선언 → MSVC goto 점프 문제 회피
    auto s1 = loaded.findById("S-001");
    ASSERT_TRUE(s1.has_value());
    EXPECT_EQ(s1->name,             "실리콘 웨이퍼");
    EXPECT_EQ(s1->pureQuantity,     480);
    EXPECT_EQ(s1->reservedQuantity, 20);
    EXPECT_DOUBLE_EQ(s1->yield,     0.92);
    EXPECT_DOUBLE_EQ(s1->cycleTime, 0.5);
}

// T8-2: Order save/load 왕복
TEST_F(PersistenceTest, OrderRepo_SaveLoad_RoundTrip) {
    {
        OrderRepository repo(ORDER_PATH);
        repo.add(makeOrderP8("ORD0001", "S-001", 100, OrderStatus::CONFIRMED, 20, "2026-06-12 09:00:00"));
    }
    OrderRepository loaded(ORDER_PATH);
    EXPECT_EQ(loaded.count(), 1);
    auto o = loaded.findById("ORD0001");
    ASSERT_TRUE(o.has_value());
    EXPECT_EQ(o->sampleId,            "S-001");
    EXPECT_EQ(o->quantity,            100);
    EXPECT_EQ(o->status,              OrderStatus::CONFIRMED);
    EXPECT_EQ(o->productionShortage,  20);
    EXPECT_EQ(o->productionStartedAt, "2026-06-12 09:00:00");
}

// T8-3: Sample JSON 키 이름 검증 (raw string search — MSVC nlohmann scope 회피)
TEST_F(PersistenceTest, SampleRepo_JsonKeys) {
    {
        SampleRepository repo(SAMPLE_PATH);
        repo.add(makeSampleP8("S-001", "테스트시료", 100));
    }
    std::ifstream ifs(SAMPLE_PATH);
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("\"id\""),               std::string::npos);
    EXPECT_NE(content.find("\"name\""),             std::string::npos);
    EXPECT_NE(content.find("\"pureQuantity\""),     std::string::npos);
    EXPECT_NE(content.find("\"reservedQuantity\""), std::string::npos);
    EXPECT_NE(content.find("\"yield\""),            std::string::npos);
    EXPECT_NE(content.find("\"cycleTime\""),        std::string::npos);
    EXPECT_NE(content.find("\"registeredAt\""),     std::string::npos);
}

// T8-4: Order JSON 키 이름 검증 (productionShortage, productionStartedAt 포함)
TEST_F(PersistenceTest, OrderRepo_JsonKeys) {
    {
        OrderRepository repo(ORDER_PATH);
        repo.add(makeOrderP8("ORD0001", "S-001", 50, OrderStatus::PRODUCING, 30, "2026-06-12 10:00:00"));
    }
    std::ifstream ifs(ORDER_PATH);
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("\"id\""),                   std::string::npos);
    EXPECT_NE(content.find("\"sampleId\""),             std::string::npos);
    EXPECT_NE(content.find("\"customerName\""),         std::string::npos);
    EXPECT_NE(content.find("\"quantity\""),             std::string::npos);
    EXPECT_NE(content.find("\"status\""),               std::string::npos);
    EXPECT_NE(content.find("\"orderedAt\""),            std::string::npos);
    EXPECT_NE(content.find("\"requiredProduction\""),   std::string::npos);
    EXPECT_NE(content.find("\"productionShortage\""),   std::string::npos);
    EXPECT_NE(content.find("\"productionStartedAt\""),  std::string::npos);
}

// T8-5: 존재하지 않는 경로로 생성 → 빈 상태
TEST_F(PersistenceTest, SampleRepo_EmptyOnNewPath) {
    SampleRepository repo(SAMPLE_PATH);
    EXPECT_EQ(repo.count(), 0);
}

// T8-6: 존재하지 않는 경로로 생성 → 빈 상태
TEST_F(PersistenceTest, OrderRepo_EmptyOnNewPath) {
    OrderRepository repo(ORDER_PATH);
    EXPECT_EQ(repo.count(), 0);
}

// T8-7: Sample update 후 재로드 반영
TEST_F(PersistenceTest, SampleRepo_UpdatePersists) {
    {
        SampleRepository repo(SAMPLE_PATH);
        repo.add(makeSampleP8("S-001", "테스트시료", 100, 0, 0.90, 0.5));
        auto s = repo.findById("S-001").value();
        s.pureQuantity = 200;
        s.yield        = 0.95;
        repo.update(s);
    }
    SampleRepository loaded(SAMPLE_PATH);
    auto s = loaded.findById("S-001");
    ASSERT_TRUE(s.has_value());
    EXPECT_EQ(s->pureQuantity, 200);
    EXPECT_DOUBLE_EQ(s->yield, 0.95);
}

// T8-8: Order update(상태 변경) 후 재로드 반영
TEST_F(PersistenceTest, OrderRepo_UpdatePersists) {
    {
        OrderRepository repo(ORDER_PATH);
        repo.add(makeOrderP8("ORD0001", "S-001", 100, OrderStatus::RESERVED));
        auto o = repo.findById("ORD0001").value();
        o.status = OrderStatus::CONFIRMED;
        repo.update(o);
    }
    OrderRepository loaded(ORDER_PATH);
    auto o = loaded.findById("ORD0001");
    ASSERT_TRUE(o.has_value());
    EXPECT_EQ(o->status, OrderStatus::CONFIRMED);
}

// T8-9: 5가지 OrderStatus 직렬화/역직렬화 왕복
TEST_F(PersistenceTest, OrderRepo_AllStatusRoundTrip) {
    {
        OrderRepository repo(ORDER_PATH);
        repo.add(makeOrderP8("O1", "S-001", 10, OrderStatus::RESERVED));
        repo.add(makeOrderP8("O2", "S-001", 10, OrderStatus::REJECTED));
        repo.add(makeOrderP8("O3", "S-001", 10, OrderStatus::PRODUCING));
        repo.add(makeOrderP8("O4", "S-001", 10, OrderStatus::CONFIRMED));
        repo.add(makeOrderP8("O5", "S-001", 10, OrderStatus::RELEASE));
    }
    OrderRepository loaded(ORDER_PATH);
    auto o1 = loaded.findById("O1");
    auto o2 = loaded.findById("O2");
    auto o3 = loaded.findById("O3");
    auto o4 = loaded.findById("O4");
    auto o5 = loaded.findById("O5");
    ASSERT_TRUE(o1 && o2 && o3 && o4 && o5);
    EXPECT_EQ(o1->status, OrderStatus::RESERVED);
    EXPECT_EQ(o2->status, OrderStatus::REJECTED);
    EXPECT_EQ(o3->status, OrderStatus::PRODUCING);
    EXPECT_EQ(o4->status, OrderStatus::CONFIRMED);
    EXPECT_EQ(o5->status, OrderStatus::RELEASE);
}

// T8-10: Sample remove 후 재로드 반영
TEST_F(PersistenceTest, SampleRepo_RemovePersists) {
    {
        SampleRepository repo(SAMPLE_PATH);
        repo.add(makeSampleP8("S-001", "시료1", 100));
        repo.add(makeSampleP8("S-002", "시료2", 200));
        repo.remove("S-001");
    }
    SampleRepository loaded(SAMPLE_PATH);
    EXPECT_EQ(loaded.count(), 1);
    EXPECT_FALSE(loaded.findById("S-001").has_value());
    EXPECT_TRUE(loaded.findById("S-002").has_value());
}

// T8-11: 재로드 후 FIFO 순서 보장
TEST_F(PersistenceTest, OrderRepo_FifoPreservedAfterLoad) {
    {
        OrderRepository repo(ORDER_PATH);
        repo.add(makeOrderP8("O-A", "S-001", 10, OrderStatus::RESERVED));
        repo.add(makeOrderP8("O-B", "S-001", 10, OrderStatus::RESERVED));
        repo.add(makeOrderP8("O-C", "S-001", 10, OrderStatus::RESERVED));
    }
    OrderRepository loaded(ORDER_PATH);
    auto all = loaded.findAll();
    ASSERT_EQ(all.size(), 3u);
    EXPECT_EQ(all[0].id, "O-A");
    EXPECT_EQ(all[1].id, "O-B");
    EXPECT_EQ(all[2].id, "O-C");
}

#endif
