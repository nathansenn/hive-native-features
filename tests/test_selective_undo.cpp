/**
 * Catalogue #43 — selective undo: field-level account_balance restore.
 */
#include "hive_native/perf/selective_undo.hpp"
#include <iostream>
#include <string>

static int g_failed = 0, g_passed = 0;
#define CHECK(c) do { if(c) ++g_passed; else { ++g_failed; std::cerr << "FAIL " << __LINE__ << " " #c "\n"; } } while(0)

static void test_adjust_then_rollback_restores() {
   using namespace hive_native::perf;
   balance_map balances;
   balances["alice"] = account_balance{1000, 200};
   balances["bob"]   = account_balance{500, 50};

   undo_session session(balances);

   // Transfer 100 HIVE alice → bob under the session
   adjust_balance(session, balances, "alice", -100, 0);
   adjust_balance(session, balances, "bob",     100, 0);

   CHECK(balances["alice"].hive == 900);
   CHECK(balances["bob"].hive   == 600);
   CHECK(session.size() == 2);

   session.rollback();

   CHECK(balances["alice"].hive == 1000);
   CHECK(balances["alice"].hbd  == 200);
   CHECK(balances["bob"].hive   == 500);
   CHECK(balances["bob"].hbd    == 50);
   CHECK(session.empty());
}

static void test_hbd_field_only() {
   using namespace hive_native::perf;
   balance_map balances;
   balances["carol"] = account_balance{10, 1000};

   undo_session session(balances);
   const int64_t old_hive = balances["carol"].hive;
   const int64_t old_hbd  = balances["carol"].hbd;
   session.push_balance_change("carol", old_hive, old_hbd);
   balances["carol"].hbd -= 250; // only HBD mutates

   CHECK(balances["carol"].hive == 10);
   CHECK(balances["carol"].hbd  == 750);

   session.rollback();
   CHECK(balances["carol"].hive == 10);
   CHECK(balances["carol"].hbd  == 1000);
}

static void test_multiple_pushes_lifo() {
   using namespace hive_native::perf;
   balance_map balances;
   balances["dave"] = account_balance{100, 0};

   undo_session session(balances);
   // two sequential mutations — each records prior fields
   session.push_balance_change("dave", 100, 0);
   balances["dave"].hive = 90;
   session.push_balance_change("dave", 90, 0);
   balances["dave"].hive = 80;

   CHECK(balances["dave"].hive == 80);
   CHECK(session.size() == 2);

   session.rollback();
   // reverse restore: 80→90 then 90→100
   CHECK(balances["dave"].hive == 100);
   CHECK(session.empty());
}

static void test_commit_keeps_mutations() {
   using namespace hive_native::perf;
   balance_map balances;
   balances["erin"] = account_balance{50, 25};

   undo_session session(balances);
   adjust_balance(session, balances, "erin", -10, 5);
   CHECK(balances["erin"].hive == 40);
   CHECK(balances["erin"].hbd  == 30);

   session.commit();
   CHECK(session.empty());
   CHECK(balances["erin"].hive == 40);
   CHECK(balances["erin"].hbd  == 30);
}

static void test_push_api_explicit() {
   using namespace hive_native::perf;
   balance_map balances;
   balances["frank"] = account_balance{1, 2};

   undo_session session(balances);
   session.push_balance_change("frank", /*old_hive=*/1, /*old_hbd=*/2);
   balances["frank"] = account_balance{999, 888};
   session.rollback();
   CHECK(balances["frank"].hive == 1);
   CHECK(balances["frank"].hbd  == 2);
}

int main() {
   test_adjust_then_rollback_restores();
   test_hbd_field_only();
   test_multiple_pushes_lifo();
   test_commit_keeps_mutations();
   test_push_api_explicit();
   std::cout << "selective_undo_passed=" << g_passed << " failed=" << g_failed << "\n";
   return g_failed ? 1 : 0;
}
