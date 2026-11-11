#include "gtest/gtest.h"
#include "currency_core/currency_basic.h"
#include "currency_core/currency_format_utils.h"

using namespace currency;
 
TEST(attachment_info_validation, allows_partial_coverage_for_intermediate_inputs)
{
  transaction tx = AUTO_VAL_INIT(tx);
 
  tx_comment c1;
  c1.comment = "first attachment";
  tx.attachment.push_back(c1);
 
  std::vector<extra_v> test_extra;
  bool r = add_attachments_info_to_extra(test_extra, tx.attachment);
  ASSERT_TRUE(r);
 
  tx_comment c2;
  c2.comment = "second attachment";
  tx.attachment.push_back(c2);
  ASSERT_EQ(tx.attachment.size(), 2U);
 
  r = validate_attachment_info(test_extra, tx.attachment, true);
  ASSERT_TRUE(r) << "validate_attachment_info should allow eai.cnt < attachments.size() " << "for intermediate inputs (allow_no_info=true)";
}
 
TEST(attachment_info_validation, accepts_correctly_signed_attachments)
{
  transaction tx = AUTO_VAL_INIT(tx);
 
  tx_comment c1;
  c1.comment = "comment 1";
  tx.attachment.push_back(c1);
 
  tx_comment c2;
  c2.comment = "comment 2";
  tx.attachment.push_back(c2);
 
  tx_service_attachment tsa;
  tsa.service_id = "test_service";
  tsa.body = "test body";
  tx.attachment.push_back(tsa);
 
  ASSERT_EQ(tx.attachment.size(), 3U);
 
  bool r = add_attachments_info_to_extra(tx.extra, tx.attachment);
  ASSERT_TRUE(r);
 
  r = validate_attachment_info(tx.extra, tx.attachment, false);
  ASSERT_TRUE(r) << "final validation should pass when all attachments are covered";
 
  r = validate_attachment_info(tx.extra, tx.attachment, true);
  ASSERT_TRUE(r) << "intermediate validation should pass when all attachments are covered";
}
 
TEST(attachment_info_validation, handles_empty_attachments)
{
  transaction tx = AUTO_VAL_INIT(tx);
 
  bool r = validate_attachment_info(tx.extra, tx.attachment, false);
  ASSERT_TRUE(r) << "empty attachments with no info should pass";
}
 
TEST(attachment_info_validation, rejects_eai_with_empty_attachments)
{
  transaction tx = AUTO_VAL_INIT(tx);
 
  tx_comment c1;
  c1.comment = "test";
  tx.attachment.push_back(c1);
 
  bool r = add_attachments_info_to_extra(tx.extra, tx.attachment);
  ASSERT_TRUE(r);
 
  tx.attachment.clear();
 
  r = validate_attachment_info(tx.extra, tx.attachment, false);
  ASSERT_FALSE(r) << "should reject when eai is present but attachments are empty";
}
 
TEST(attachment_info_validation, txid_unchanged_after_tail_addition)
{
  transaction tx = AUTO_VAL_INIT(tx);
 
  tx_comment c1;
  c1.comment = "original comment";
  tx.attachment.push_back(c1);
 
  bool r = add_attachments_info_to_extra(tx.extra, tx.attachment);
  ASSERT_TRUE(r);
 
  crypto::hash h_before = get_transaction_hash(tx);
 
  tx_comment c2;
  c2.comment = "bad tail attachment";
  tx.attachment.push_back(c2);
 
  crypto::hash h_after = get_transaction_hash(tx);
 
  ASSERT_EQ(h_before, h_after) << "transaction hash should be unchanged when adding tail attachments, "
    << "which is why validation must reject such transactions";
}

TEST(attachment_info_validation, rejects_unsigned_tail_attachments)
{
  transaction tx = AUTO_VAL_INIT(tx);
 
  tx_comment c1;
  c1.comment = "original comment";
  tx.attachment.push_back(c1);
 
  bool r = add_attachments_info_to_extra(tx.extra, tx.attachment);
  ASSERT_TRUE(r) << "add_attachments_info_to_extra should succeed";
 
  r = validate_attachment_info(tx.extra, tx.attachment, false);
  ASSERT_TRUE(r) << "validate_attachment_info should pass when eai.cnt == attachments.size()";
 
  tx_comment c2;
  c2.comment = "bad tail attachment";
  tx.attachment.push_back(c2);
  ASSERT_EQ(tx.attachment.size(), 2U);
 
  r = validate_attachment_info(tx.extra, tx.attachment, false);
  ASSERT_FALSE(r) << "validate_attachment_info MUST reject tx with eai.cnt < attachments.size() "
    << "in final validation allow_no_info=false to prevent unsigned tail";
}