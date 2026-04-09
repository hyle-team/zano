#include "gtest/gtest.h"
#include "currency_core/currency_format_utils_abstract.h"
#include "currency_core/currency_format_utils_transactions.h"

TEST(prepare_outputs_for_key_offsets, size_lower_than_2)
{
  for (size_t size{}; size < 2; ++size)
  {
    std::vector<currency::tx_source_entry::output_entry> outputs(size);
    size_t index{SIZE_MAX};

    ASSERT_EQ(currency::prepare_outputs_entries_for_key_offsets(outputs, 0, index), outputs);
    ASSERT_EQ(index, 0);
  }
}

TEST(prepare_outputs_for_key_offsets, pass_index_greater_or_equal_to_length_of_outputs_container)
{
  const std::vector<currency::tx_source_entry::output_entry> outputs(2);

  {
    auto index{crypto::rand<size_t>()};
    const auto index_before_preparing_outputs{index};

    ASSERT_TRUE(currency::prepare_outputs_entries_for_key_offsets(outputs, outputs.size(), index).empty());
    ASSERT_EQ(index, index_before_preparing_outputs);
  }

  {
    auto index{crypto::rand<size_t>()};
    const auto index_before_preparing_outputs{index};

    ASSERT_TRUE(currency::prepare_outputs_entries_for_key_offsets(outputs, outputs.size() + 1, index).empty());
    ASSERT_EQ(index, index_before_preparing_outputs);
  }

  {
    auto index{crypto::rand<size_t>()};
    const auto index_before_preparing_outputs{index};

    ASSERT_TRUE(currency::prepare_outputs_entries_for_key_offsets(outputs, SIZE_MAX - 1, index).empty());
    ASSERT_EQ(index, index_before_preparing_outputs);
  }

  {
    auto index{crypto::rand<size_t>()};
    const auto index_before_preparing_outputs{index};

    ASSERT_EQ(currency::prepare_outputs_entries_for_key_offsets(outputs, SIZE_MAX, index), outputs);
    ASSERT_EQ(index, index_before_preparing_outputs);
  }
}

TEST(prepare_outputs_for_key_offsets, uint64_reference_lower_than_by_id_reference)
{
  std::vector<currency::tx_source_entry::output_entry> outputs{}, expected_prepared_outputs{};
  size_t index{};

  outputs.reserve(2);
  outputs.emplace_back(currency::ref_by_id{}, currency::null_pkey);
  outputs.emplace_back(0, currency::null_pkey);
  expected_prepared_outputs.reserve(outputs.size());
  expected_prepared_outputs.emplace_back(0, currency::null_pkey);
  expected_prepared_outputs.emplace_back(currency::ref_by_id{}, currency::null_pkey);
  ASSERT_EQ(outputs.size(), expected_prepared_outputs.size());
  ASSERT_EQ(currency::prepare_outputs_entries_for_key_offsets(outputs, 0, index), expected_prepared_outputs);
  ASSERT_EQ(index, 1);
}

TEST(prepare_outputs_for_key_offsets, subsequence_of_ref_by_id_references_is_not_changed)
{
  {
    std::vector<currency::tx_source_entry::output_entry> outputs{}, expected_prepared_outputs{};
    size_t index{};

    outputs.reserve(6);
    outputs.emplace_back(3, currency::null_pkey);
    outputs.emplace_back(currency::ref_by_id{currency::null_hash, 1}, currency::null_pkey);
    outputs.emplace_back(currency::ref_by_id{currency::null_hash, 0}, currency::null_pkey);
    outputs.emplace_back(2, currency::null_pkey);
    outputs.emplace_back(currency::ref_by_id{currency::null_hash, 3}, currency::null_pkey);
    outputs.emplace_back(1, currency::null_pkey);
    expected_prepared_outputs.reserve(outputs.size());
    expected_prepared_outputs.emplace_back(1, currency::null_pkey);
    expected_prepared_outputs.emplace_back(1, currency::null_pkey);
    expected_prepared_outputs.emplace_back(1, currency::null_pkey);
    expected_prepared_outputs.emplace_back(currency::ref_by_id{currency::null_hash, 1}, currency::null_pkey);
    expected_prepared_outputs.emplace_back(currency::ref_by_id{currency::null_hash, 0}, currency::null_pkey);
    expected_prepared_outputs.emplace_back(currency::ref_by_id{currency::null_hash, 3}, currency::null_pkey);
    ASSERT_EQ(outputs.size(), expected_prepared_outputs.size());
    ASSERT_EQ(currency::prepare_outputs_entries_for_key_offsets(outputs, 0, index), expected_prepared_outputs);
    ASSERT_EQ(index, 2);
  }

  {
    std::vector<currency::tx_source_entry::output_entry> outputs{}, expected_prepared_outputs{};
    size_t index{};

    outputs.reserve(5);
    outputs.emplace_back(currency::ref_by_id{currency::null_hash, 1}, currency::null_pkey);
    outputs.emplace_back(currency::ref_by_id{currency::null_hash, 0}, currency::null_pkey);
    outputs.emplace_back(currency::ref_by_id{currency::null_hash, 3}, currency::null_pkey);
    outputs.emplace_back(1, currency::null_pkey);
    outputs.emplace_back(0, currency::null_pkey);
    expected_prepared_outputs.reserve(outputs.size());
    expected_prepared_outputs.emplace_back(0, currency::null_pkey);
    expected_prepared_outputs.emplace_back(1, currency::null_pkey);
    expected_prepared_outputs.emplace_back(currency::ref_by_id{currency::null_hash, 1}, currency::null_pkey);
    expected_prepared_outputs.emplace_back(currency::ref_by_id{currency::null_hash, 0}, currency::null_pkey);
    expected_prepared_outputs.emplace_back(currency::ref_by_id{currency::null_hash, 3}, currency::null_pkey);
    ASSERT_EQ(outputs.size(), expected_prepared_outputs.size());
    ASSERT_EQ(currency::prepare_outputs_entries_for_key_offsets(outputs, 0, index), expected_prepared_outputs);
    ASSERT_EQ(outputs.at(0), expected_prepared_outputs.at(2));
    ASSERT_EQ(index, 2);
  }
}

TEST(prepare_outputs_for_key_offsets, index_will_not_be_changed_if_old_index_is_size_max)
{
  std::vector<currency::tx_source_entry::output_entry> outputs{}, expected_prepared_outputs{};
  auto index{crypto::rand<size_t>()};
  const auto index_before_preparing_outputs{index};

  outputs.reserve(5);
  outputs.emplace_back(currency::ref_by_id{currency::null_hash, 1}, currency::null_pkey);
  outputs.emplace_back(currency::ref_by_id{currency::null_hash, 0}, currency::null_pkey);
  outputs.emplace_back(currency::ref_by_id{currency::null_hash, 3}, currency::null_pkey);
  outputs.emplace_back(1, currency::null_pkey);
  outputs.emplace_back(0, currency::null_pkey);
  expected_prepared_outputs.reserve(outputs.size());
  expected_prepared_outputs.emplace_back(0, currency::null_pkey);
  expected_prepared_outputs.emplace_back(1, currency::null_pkey);
  expected_prepared_outputs.emplace_back(currency::ref_by_id{currency::null_hash, 1}, currency::null_pkey);
  expected_prepared_outputs.emplace_back(currency::ref_by_id{currency::null_hash, 0}, currency::null_pkey);
  expected_prepared_outputs.emplace_back(currency::ref_by_id{currency::null_hash, 3}, currency::null_pkey);
  ASSERT_EQ(outputs.size(), expected_prepared_outputs.size());
  ASSERT_EQ(currency::prepare_outputs_entries_for_key_offsets(outputs, SIZE_MAX, index), expected_prepared_outputs);
  ASSERT_EQ(index, index_before_preparing_outputs);
}
