// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include "include_base_utils.h"
#include "currency_core/currency_format_utils.h"

bool if_alt_chain_stronger(const currency::wide_difficulty_type& pos, const currency::wide_difficulty_type& pow)
{
  currency::difficulties main_cumul_diff;
  main_cumul_diff.pos_diff = 400000;
  main_cumul_diff.pow_diff = 4000;
  currency::difficulties alt_cumul_diff;
  alt_cumul_diff.pow_diff = pow;
  alt_cumul_diff.pos_diff = pos;
  static currency::wide_difficulty_type difficulty_pos_at_split_point = 400000;
  static currency::wide_difficulty_type difficulty_pow_at_split_point = 4000;
  boost::multiprecision::uint1024_t main = currency::get_a_to_b_relative_cumulative_difficulty(difficulty_pos_at_split_point, difficulty_pow_at_split_point, main_cumul_diff, alt_cumul_diff);
  boost::multiprecision::uint1024_t alt = currency::get_a_to_b_relative_cumulative_difficulty(difficulty_pos_at_split_point, difficulty_pow_at_split_point, alt_cumul_diff, main_cumul_diff);
  if (alt > main)
    return true;
  return false;
}

TEST(fork_choice_rule_test, fork_choice_rule_test_1)
{
//   std::stringstream ss;
//   for (uint64_t pos = 100000; pos < 1000001; pos += 10000)
//   {
//     for (uint64_t pow = 100; pow < 18000; pow += 100)
//     {
//       bool r = if_alt_chain_stronger(pos, pow);
//       if(r)
//         ss << pos << "\t" << pow << std::endl;
//         //ss << pos << "\t" << pow << "\t" << (r ? "1" : "0") << std::endl;
//       
// 
//     }
//   }
//   bool r = epee::file_io_utils::save_string_to_file("stat.txt", ss.str());
  bool res = false;
  res = if_alt_chain_stronger(1000000, 1000);
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger(1000000, 1500);
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger(800000, 1700);
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger(800000, 2000);
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger(600000, 2200);
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger(600000, 2800);
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger(400000, 3999);
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger(400000, 4001);
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger(200000, 7000);
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger(200000, 7700);
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger(200000, 7000);
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger(200000, 7700);
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger(100000, 10000);
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger(200000, 14000);
  ASSERT_TRUE(res);
}


bool if_alt_chain_stronger_hf4(const currency::wide_difficulty_type& pos, const currency::wide_difficulty_type& pow)
{
  currency::difficulties main_cumul_diff;
  main_cumul_diff.pos_diff.assign("1605973467987652534120344647");
  main_cumul_diff.pow_diff.assign("3011264554002844981");
  currency::difficulties alt_cumul_diff;
  alt_cumul_diff.pow_diff = pow;
  alt_cumul_diff.pos_diff = pos;
  currency::wide_difficulty_type difficulty_pos_at_split_point = main_cumul_diff.pos_diff;
  currency::wide_difficulty_type difficulty_pow_at_split_point = main_cumul_diff.pow_diff;
  boost::multiprecision::uint1024_t main = currency::get_a_to_b_relative_cumulative_difficulty_hf4(difficulty_pos_at_split_point, difficulty_pow_at_split_point, main_cumul_diff, alt_cumul_diff);
  boost::multiprecision::uint1024_t alt = currency::get_a_to_b_relative_cumulative_difficulty_hf4(difficulty_pos_at_split_point, difficulty_pow_at_split_point, alt_cumul_diff, main_cumul_diff);
  if (alt > main)
    return true;
  return false;
}


bool less_or_equal(const currency::wide_difficulty_type& pos, const currency::wide_difficulty_type& pow) {
  // Example predicate: less or equal to a certain target
  return !if_alt_chain_stronger_hf4(pos, pow);//mid <= target;
}

currency::wide_difficulty_type binary_search_custom(currency::wide_difficulty_type low, currency::wide_difficulty_type high, std::function<bool(const currency::wide_difficulty_type&)> less_or_equal) {
  while (low < high) 
  {
    currency::wide_difficulty_type mid = low + (high - low) / 2;

    if (less_or_equal(mid)) {
      low = mid + 1;
      LOG_PRINT_L0("mid: less_or_eq_true: " << mid);
    }
    else {
      high = mid;
      LOG_PRINT_L0("mid: less_or_eq_false: " << mid);
    }
  }
  LOG_PRINT_L0("ret_low: " << low);
  return low;
}

currency::wide_difficulty_type find_break_point(const currency::wide_difficulty_type& pos, const currency::wide_difficulty_type& pow)
{
  currency::wide_difficulty_type last_takeover_pow = binary_search_custom(0, pow, [&](const currency::wide_difficulty_type& pow_mid) {
      return less_or_equal(pos, pow_mid);
    });

  return last_takeover_pow;
  /*for (currency::wide_difficulty_type pow_now = pow; pow_now != 0; pow_now--)
  {
    if(!if_alt_chain_stronger_hf4(pos, pow))
    {
      if (pow == pow_now)
        return 0;
      return last_takeover_pow;
    }
  }*/
}

TEST(fork_choice_rule_test, fork_choice_rule_test_hf4)
{
  std::stringstream ss;
  currency::wide_difficulty_type pos_start, pos_end, pos_step, pos_diveder;
  pos_start.assign ("500000000000000000000000000");//pos_start.assign("16059734679876525341203446");
  //                1605973467987652534120344647  <- split point
  pos_end.assign  ("2500000000000000000000000000");//pos_end.assign  ("16059734679876525341203446400");
  pos_step.assign ("5000000000000000000000000");
  pos_diveder.assign("100000000000000000000000");

  currency::wide_difficulty_type pow_start, pow_end, pow_step, pow_diveder;
  pow_start.assign("30112645540028449");   //pow_start.assign("301126455400284498");
  //                  3011264554002844981     <- split point
  pow_end.assign  ("301126455400284498100");//  pow_end.assign  ("30112645540028449810");
  pow_step.assign   ("5000000000000000");
  pow_diveder.assign("1000000000000000");


  /************************************************************************
  for (currency::wide_difficulty_type pos = pos_start; pos < pos_end; pos += pos_step)
  {
    for (currency::wide_difficulty_type pow = pow_start; pow < pow_end; pow += pow_step)
    {
      bool r = if_alt_chain_stronger_hf4(pos, pow);
      if (r)
      {
        ss << pos / pos_diveder << "\t" << pow / pow_diveder << std::endl;
        break;
      }
      //ss << pos << "\t" << pow << "\t" << (r ? "1" : "0") << std::endl;     
    }
  }
  bool r = epee::file_io_utils::save_string_to_file("stat_hf4.txt", ss.str());
  
  latest results:
  https://docs.google.com/spreadsheets/d/e/2PACX-1vSan0_LNlMTzFXTUPc1CxAqeV4RPh-19YLicpNIjPxBcW2BLMjQK06A_tL4GdckXrYotRDD-FlCONvr/pubhtml

    
  ************************************************************************/

  /*
  std::vector<std::pair<uint64_t, uint64_t>> break_points_init = {
    {8100, 173895},
    {8150, 102145},
{8200, 72570},
{8250, 56430},
{8300, 46265},
{8350, 39275},
{8400, 34170},
{8450, 30285},
{8500, 27225},
{8550, 24750},
{8600, 22715},
{8650, 21005},
{8700, 19550},
{8750, 18295},
{8800, 17205},
{8850, 16250},
{8900, 15400},
{8950, 14645},
{9000, 13970},
{9050, 13360},
{9100, 12805},
{9150, 12300},
{9200, 11840},
{9250, 11415},
{9300, 11025},
{9350, 10665},
{9400, 10330},
{9450, 10020},
{9500, 9730},
{9550, 9460},
{9600, 9210},
{9650, 8970},
{9700, 8745},
{9750, 8535},
{9800, 8340},
{9850, 8150},
{9900, 7975},
{9950, 7805},
{10000, 7645},
{10050, 7495},
{10100, 7350},
{10150, 7210},
{10200, 7080},
{10250, 6955},
{10300, 6835},
{10350, 6720},
{10400, 6610},
{10450, 6505},
{10500, 6405},
{10550, 6305},
{10600, 6210},
{10650, 6120},
{10700, 6035},
{10750, 5955},
{10800, 5870},
{10850, 5795},
{10900, 5720},
{10950, 5650},
{11000, 5580},
{11050, 5510},
{11100, 5445},
{11150, 5385},
{11200, 5320},
{11250, 5265},
{11300, 5205},
{11350, 5150},
{11400, 5095},
{11450, 5045},
{11500, 4990},
{11550, 4945},
{11600, 4895},
{11650, 4850},
{11700, 4800},
{11750, 4760},
{11800, 4715},
{11850, 4675},
{11900, 4630},
{11950, 4590},
{12000, 4555},
{12050, 4515},
{12100, 4480},
{12150, 4440},
{12200, 4405},
{12250, 4375},
{12300, 4340},
{12350, 4305},
{12400, 4275},
{12450, 4245},
{12500, 4215},
{12550, 4185},
{12600, 4155},
{12650, 4125},
{12700, 4095},
{12750, 4070},
{12800, 4045},
{12850, 4015},
{12900, 3990},
{12950, 3965},
{13000, 3940},
{13050, 3915},
{13100, 3895},
{13150, 3870},
{13200, 3845},
{13250, 3825},
{13300, 3800},
{13350, 3780},
{13400, 3760},
{13450, 3740},
{13500, 3720},
{13550, 3700},
{13600, 3680},
{13650, 3660},
{13700, 3640},
{13750, 3620},
{13800, 3605},
{13850, 3585},
{13900, 3570},
{13950, 3550},
{14000, 3535},
{14050, 3515},
{14100, 3500},
{14150, 3485},
{14200, 3465},
{14250, 3450},
{14300, 3435},
{14350, 3420},
{14400, 3405},
{14450, 3390},
{14500, 3375},
{14550, 3360},
{14600, 3350},
{14650, 3335},
{14700, 3320},
{14750, 3305},
{14800, 3295},
{14850, 3280},
{14900, 3270},
{14950, 3255},
{15000, 3245},
{15050, 3230},
{15100, 3220},
{15150, 3205},
{15200, 3195},
{15250, 3185},
{15300, 3170},
{15350, 3160},
{15400, 3150},
{15450, 3135},
{15500, 3125},
{15550, 3115},
{15600, 3105},
{15650, 3095},
{15700, 3085},
{15750, 3075},
{15800, 3065},
{15850, 3055},
{15900, 3045},
{15950, 3035},
{16000, 3025},
{16050, 3015},
{16100, 3005},
{16150, 2995},
{16200, 2990},
{16250, 2980},
{16300, 2970},
{16350, 2960},
{16400, 2950},
{16450, 2945},
{16500, 2935},
{16550, 2925},
{16600, 2920},
{16650, 2910},
{16700, 2900},
{16750, 2895},
{16800, 2885},
{16850, 2880},
{16900, 2870},
{16950, 2865},
{17000, 2855},
{17050, 2850},
{17100, 2840},
{17150, 2835},
{17200, 2825},
{17250, 2820},
{17300, 2810},
{17350, 2805},
{17400, 2800},
{17450, 2790},
{17500, 2785},
{17550, 2780},
{17600, 2770},
{17650, 2765},
{17700, 2760},
{17750, 2750},
{17800, 2745},
{17850, 2740},
{17900, 2735},
{17950, 2725},
{18000, 2720},
{18050, 2715},
{18100, 2710},
{18150, 2705},
{18200, 2695},
{18250, 2690},
{18300, 2685},
{18350, 2680},
{18400, 2675},
{18450, 2670},
{18500, 2665},
{18550, 2655},
{18600, 2650},
{18650, 2645},
{18700, 2640},
{18750, 2635},
{18800, 2630},
{18850, 2625},
{18900, 2620},
{18950, 2615},
{19000, 2610},
{19050, 2605},
{19100, 2600},
{19150, 2595},
{19200, 2590},
{19250, 2585},
{19300, 2580},
{19350, 2575},
{19400, 2570},
{19450, 2565},
{19500, 2560},
{19550, 2555},
{19600, 2555},
{19650, 2550},
{19700, 2545},
{19750, 2540},
{19800, 2535},
{19850, 2530},
{19900, 2525},
{19950, 2520},
{20000, 2520},
{20050, 2515},
{20100, 2510},
{20150, 2505},
{20200, 2500},
{20250, 2495},
{20300, 2495},
{20350, 2490},
{20400, 2485},
{20450, 2480},
{20500, 2480},
{20550, 2475},
{20600, 2470},
{20650, 2465},
{20700, 2460},
{20750, 2460},
{20800, 2455},
{20850, 2450},
{20900, 2445},
{20950, 2445},
{21000, 2440},
{21050, 2435},
{21100, 2435},
{21150, 2430},
{21200, 2425},
{21250, 2425},
{21300, 2420},
{21350, 2415},
{21400, 2410},
{21450, 2410},
{21500, 2405},
{21550, 2400},
{21600, 2400},
{21650, 2395},
{21700, 2390},
{21750, 2390},
{21800, 2385},
{21850, 2385},
{21900, 2380},
{21950, 2375},
{22000, 2375},
{22050, 2370},
{22100, 2365},
{22150, 2365},
{22200, 2360},
{22250, 2360},
{22300, 2355},
{22350, 2350},
{22400, 2350},
{22450, 2345},
{22500, 2345},
{22550, 2340},
{22600, 2340},
{22650, 2335},
{22700, 2330},
{22750, 2330},
{22800, 2325},
{22850, 2325},
{22900, 2320},
{22950, 2320},
{23000, 2315},
{23050, 2315},
{23100, 2310},
{23150, 2310},
{23200, 2305},
{23250, 2300},
{23300, 2300},
{23350, 2295},
{23400, 2295},
{23450, 2290},
{23500, 2290},
{23550, 2285},
{23600, 2285},
{23650, 2280},
{23700, 2280},
{23750, 2275},
{23800, 2275},
{23850, 2270},
{23900, 2270},
{23950, 2265},
{24000, 2265},
{24050, 2265},
{24100, 2260},
{24150, 2260},
{24200, 2255},
{24250, 2255},
{24300, 2250},
{24350, 2250},
{24400, 2245},
{24450, 2245},
{24500, 2240},
{24550, 2240},
{24600, 2240},
{24650, 2235},
{24700, 2235},
{24750, 2230},
{24800, 2230},
{24850, 2225},
{24900, 2225},
{24950, 2225}
  };*/
  /*
  //std::stringstream ss;
  for (auto bp : break_points_init)
  {
    currency::wide_difficulty_type pow_takeover_point = find_break_point(pos_diveder * bp.first, (pow_diveder * bp.second) + pow_step);

    if (pow_takeover_point == 0)
    {
      LOG_ERROR("Failed");
    }
    else
    {
      //self check 
      if (if_alt_chain_stronger_hf4(pos_diveder * bp.first, pow_takeover_point - 1) == if_alt_chain_stronger_hf4(pos_diveder * bp.first, pow_takeover_point))
      {
        bool rfrf = if_alt_chain_stronger_hf4(pos_diveder * bp.first, pow_takeover_point);
        rfrf = if_alt_chain_stronger_hf4(pos_diveder * bp.first, pow_takeover_point+1);
        rfrf = if_alt_chain_stronger_hf4(pos_diveder * bp.first, pow_takeover_point+2);
        rfrf = if_alt_chain_stronger_hf4(pos_diveder * bp.first, pow_takeover_point+3);
        rfrf = if_alt_chain_stronger_hf4(pos_diveder * bp.first, pow_takeover_point-1);
        rfrf = if_alt_chain_stronger_hf4(pos_diveder * bp.first, pow_takeover_point-2);
        rfrf = if_alt_chain_stronger_hf4(pos_diveder * bp.first, pow_takeover_point - 3);



        LOG_ERROR("ERROR");
        return;
      }
      ss <<
        "res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type(\"" << pos_diveder * bp.first << "\"), currency::wide_difficulty_type(\"" << pow_takeover_point << "\"));" << ENDL <<
        "ASSERT_TRUE(res);" << ENDL <<
        "res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type(\"" << pos_diveder * bp.first << "\"), currency::wide_difficulty_type(\"" << pow_takeover_point - 1 << "\"));" << ENDL <<
        "ASSERT_FALSE(res);" << ENDL;
    }
  }
  bool r = epee::file_io_utils::save_string_to_file("script.txt", ss.str());
  */
  bool res = false;
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("810000000000000000000000000"), currency::wide_difficulty_type("173893610095151033482"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("810000000000000000000000000"), currency::wide_difficulty_type("173893610095151033481"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("815000000000000000000000000"), currency::wide_difficulty_type("102144604566780242768"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("815000000000000000000000000"), currency::wide_difficulty_type("102144604566780242767"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("820000000000000000000000000"), currency::wide_difficulty_type("72567987045706043158"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("820000000000000000000000000"), currency::wide_difficulty_type("72567987045706043157"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("825000000000000000000000000"), currency::wide_difficulty_type("56427184779296592563"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("825000000000000000000000000"), currency::wide_difficulty_type("56427184779296592562"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("830000000000000000000000000"), currency::wide_difficulty_type("46261521639981421968"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("830000000000000000000000000"), currency::wide_difficulty_type("46261521639981421967"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("835000000000000000000000000"), currency::wide_difficulty_type("39271311807228203073"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("835000000000000000000000000"), currency::wide_difficulty_type("39271311807228203072"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("840000000000000000000000000"), currency::wide_difficulty_type("34169670746435628497"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("840000000000000000000000000"), currency::wide_difficulty_type("34169670746435628496"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("845000000000000000000000000"), currency::wide_difficulty_type("30282322585424495014"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("845000000000000000000000000"), currency::wide_difficulty_type("30282322585424495013"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("850000000000000000000000000"), currency::wide_difficulty_type("27221836391522953727"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("850000000000000000000000000"), currency::wide_difficulty_type("27221836391522953726"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("855000000000000000000000000"), currency::wide_difficulty_type("24749755123691287729"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("855000000000000000000000000"), currency::wide_difficulty_type("24749755123691287728"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("860000000000000000000000000"), currency::wide_difficulty_type("22711271409727869596"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("860000000000000000000000000"), currency::wide_difficulty_type("22711271409727869595"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("865000000000000000000000000"), currency::wide_difficulty_type("21001505056620832872"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("865000000000000000000000000"), currency::wide_difficulty_type("21001505056620832871"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("870000000000000000000000000"), currency::wide_difficulty_type("19546877193995594351"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("870000000000000000000000000"), currency::wide_difficulty_type("19546877193995594350"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("875000000000000000000000000"), currency::wide_difficulty_type("18294243761466129083"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("875000000000000000000000000"), currency::wide_difficulty_type("18294243761466129082"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("880000000000000000000000000"), currency::wide_difficulty_type("17204261972931225543"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("880000000000000000000000000"), currency::wide_difficulty_type("17204261972931225542"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("885000000000000000000000000"), currency::wide_difficulty_type("16247183291614714294"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("885000000000000000000000000"), currency::wide_difficulty_type("16247183291614714293"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("890000000000000000000000000"), currency::wide_difficulty_type("15400096882194836021"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("890000000000000000000000000"), currency::wide_difficulty_type("15400096882194836020"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("895000000000000000000000000"), currency::wide_difficulty_type("14645071807644110435"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("895000000000000000000000000"), currency::wide_difficulty_type("14645071807644110434"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("900000000000000000000000000"), currency::wide_difficulty_type("13967873725795875372"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("900000000000000000000000000"), currency::wide_difficulty_type("13967873725795875371"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("905000000000000000000000000"), currency::wide_difficulty_type("13357058979013812170"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("905000000000000000000000000"), currency::wide_difficulty_type("13357058979013812169"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("910000000000000000000000000"), currency::wide_difficulty_type("12803322645925507695"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("910000000000000000000000000"), currency::wide_difficulty_type("12803322645925507694"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("915000000000000000000000000"), currency::wide_difficulty_type("12299021201474213830"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("915000000000000000000000000"), currency::wide_difficulty_type("12299021201474213829"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("920000000000000000000000000"), currency::wide_difficulty_type("11837817557953003092"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("920000000000000000000000000"), currency::wide_difficulty_type("11837817557953003091"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("925000000000000000000000000"), currency::wide_difficulty_type("11414413381540383189"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("925000000000000000000000000"), currency::wide_difficulty_type("11414413381540383188"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("930000000000000000000000000"), currency::wide_difficulty_type("11024344634545981648"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("930000000000000000000000000"), currency::wide_difficulty_type("11024344634545981647"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("935000000000000000000000000"), currency::wide_difficulty_type("10663823580658131959"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("935000000000000000000000000"), currency::wide_difficulty_type("10663823580658131958"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("940000000000000000000000000"), currency::wide_difficulty_type("10329615384232683380"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("940000000000000000000000000"), currency::wide_difficulty_type("10329615384232683379"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("945000000000000000000000000"), currency::wide_difficulty_type("10018940777719244821"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("945000000000000000000000000"), currency::wide_difficulty_type("10018940777719244820"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("950000000000000000000000000"), currency::wide_difficulty_type("9729398591085546371"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("950000000000000000000000000"), currency::wide_difficulty_type("9729398591085546370"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("955000000000000000000000000"), currency::wide_difficulty_type("9458903570152632164"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("955000000000000000000000000"), currency::wide_difficulty_type("9458903570152632163"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("960000000000000000000000000"), currency::wide_difficulty_type("9205636075772937886"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("960000000000000000000000000"), currency::wide_difficulty_type("9205636075772937885"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("965000000000000000000000000"), currency::wide_difficulty_type("8968001097212660715"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("965000000000000000000000000"), currency::wide_difficulty_type("8968001097212660714"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("970000000000000000000000000"), currency::wide_difficulty_type("8744594627814733132"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("970000000000000000000000000"), currency::wide_difficulty_type("8744594627814733131"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("975000000000000000000000000"), currency::wide_difficulty_type("8534175904919445023"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("975000000000000000000000000"), currency::wide_difficulty_type("8534175904919445022"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("980000000000000000000000000"), currency::wide_difficulty_type("8335644354530648558"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("980000000000000000000000000"), currency::wide_difficulty_type("8335644354530648557"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("985000000000000000000000000"), currency::wide_difficulty_type("8148020336034723158"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("985000000000000000000000000"), currency::wide_difficulty_type("8148020336034723157"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("990000000000000000000000000"), currency::wide_difficulty_type("7970428975782931705"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("990000000000000000000000000"), currency::wide_difficulty_type("7970428975782931704"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("995000000000000000000000000"), currency::wide_difficulty_type("7802086526502014600"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("995000000000000000000000000"), currency::wide_difficulty_type("7802086526502014599"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1000000000000000000000000000"), currency::wide_difficulty_type("7642288803812028782"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1000000000000000000000000000"), currency::wide_difficulty_type("7642288803812028781"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1005000000000000000000000000"), currency::wide_difficulty_type("7490401339980245977"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1005000000000000000000000000"), currency::wide_difficulty_type("7490401339980245976"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1010000000000000000000000000"), currency::wide_difficulty_type("7345850964575793868"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1010000000000000000000000000"), currency::wide_difficulty_type("7345850964575793867"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1015000000000000000000000000"), currency::wide_difficulty_type("7208118576466544449"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1015000000000000000000000000"), currency::wide_difficulty_type("7208118576466544448"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1020000000000000000000000000"), currency::wide_difficulty_type("7076732915018020514"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1020000000000000000000000000"), currency::wide_difficulty_type("7076732915018020513"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1025000000000000000000000000"), currency::wide_difficulty_type("6951265172971883569"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1025000000000000000000000000"), currency::wide_difficulty_type("6951265172971883568"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1030000000000000000000000000"), currency::wide_difficulty_type("6831324321237202029"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1030000000000000000000000000"), currency::wide_difficulty_type("6831324321237202028"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1035000000000000000000000000"), currency::wide_difficulty_type("6716553038200005653"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1035000000000000000000000000"), currency::wide_difficulty_type("6716553038200005652"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1040000000000000000000000000"), currency::wide_difficulty_type("6606624154281270723"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1040000000000000000000000000"), currency::wide_difficulty_type("6606624154281270722"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1045000000000000000000000000"), currency::wide_difficulty_type("6501237537228019549"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1045000000000000000000000000"), currency::wide_difficulty_type("6501237537228019548"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1050000000000000000000000000"), currency::wide_difficulty_type("6400117355688827656"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1050000000000000000000000000"), currency::wide_difficulty_type("6400117355688827655"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1055000000000000000000000000"), currency::wide_difficulty_type("6303009668537003243"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1055000000000000000000000000"), currency::wide_difficulty_type("6303009668537003242"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1060000000000000000000000000"), currency::wide_difficulty_type("6209680295581205260"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1060000000000000000000000000"), currency::wide_difficulty_type("6209680295581205259"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1065000000000000000000000000"), currency::wide_difficulty_type("6119912932075478362"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1065000000000000000000000000"), currency::wide_difficulty_type("6119912932075478361"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1070000000000000000000000000"), currency::wide_difficulty_type("6033507475071567420"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1070000000000000000000000000"), currency::wide_difficulty_type("6033507475071567419"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1075000000000000000000000000"), currency::wide_difficulty_type("5950278534355724202"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1075000000000000000000000000"), currency::wide_difficulty_type("5950278534355724201"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1080000000000000000000000000"), currency::wide_difficulty_type("5870054104648172855"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1080000000000000000000000000"), currency::wide_difficulty_type("5870054104648172854"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1085000000000000000000000000"), currency::wide_difficulty_type("5792674379051306677"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1085000000000000000000000000"), currency::wide_difficulty_type("5792674379051306676"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1090000000000000000000000000"), currency::wide_difficulty_type("5717990686521957375"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1090000000000000000000000000"), currency::wide_difficulty_type("5717990686521957374"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1095000000000000000000000000"), currency::wide_difficulty_type("5645864538502512988"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1095000000000000000000000000"), currency::wide_difficulty_type("5645864538502512987"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1100000000000000000000000000"), currency::wide_difficulty_type("5576166771847621697"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1100000000000000000000000000"), currency::wide_difficulty_type("5576166771847621696"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1105000000000000000000000000"), currency::wide_difficulty_type("5508776776886887265"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1105000000000000000000000000"), currency::wide_difficulty_type("5508776776886887264"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1110000000000000000000000000"), currency::wide_difficulty_type("5443581800917917808"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1110000000000000000000000000"), currency::wide_difficulty_type("5443581800917917807"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1115000000000000000000000000"), currency::wide_difficulty_type("5380476318668349354"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1115000000000000000000000000"), currency::wide_difficulty_type("5380476318668349353"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1120000000000000000000000000"), currency::wide_difficulty_type("5319361462333102716"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1120000000000000000000000000"), currency::wide_difficulty_type("5319361462333102715"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1125000000000000000000000000"), currency::wide_difficulty_type("5260144504711571647"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1125000000000000000000000000"), currency::wide_difficulty_type("5260144504711571646"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1130000000000000000000000000"), currency::wide_difficulty_type("5202738389761493946"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1130000000000000000000000000"), currency::wide_difficulty_type("5202738389761493945"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1135000000000000000000000000"), currency::wide_difficulty_type("5147061305570958556"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1135000000000000000000000000"), currency::wide_difficulty_type("5147061305570958555"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1140000000000000000000000000"), currency::wide_difficulty_type("5093036295343277653"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1140000000000000000000000000"), currency::wide_difficulty_type("5093036295343277652"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1145000000000000000000000000"), currency::wide_difficulty_type("5040590902504669188"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1145000000000000000000000000"), currency::wide_difficulty_type("5040590902504669187"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1150000000000000000000000000"), currency::wide_difficulty_type("4989656846493099358"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1150000000000000000000000000"), currency::wide_difficulty_type("4989656846493099357"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1155000000000000000000000000"), currency::wide_difficulty_type("4940169726177716516"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1155000000000000000000000000"), currency::wide_difficulty_type("4940169726177716515"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1160000000000000000000000000"), currency::wide_difficulty_type("4892068748200095632"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1160000000000000000000000000"), currency::wide_difficulty_type("4892068748200095631"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1165000000000000000000000000"), currency::wide_difficulty_type("4845296477827814285"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1165000000000000000000000000"), currency::wide_difficulty_type("4845296477827814284"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1170000000000000000000000000"), currency::wide_difficulty_type("4799798610173485225"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1170000000000000000000000000"), currency::wide_difficulty_type("4799798610173485224"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1175000000000000000000000000"), currency::wide_difficulty_type("4755523759863208997"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1175000000000000000000000000"), currency::wide_difficulty_type("4755523759863208996"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1180000000000000000000000000"), currency::wide_difficulty_type("4712423267441696058"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1180000000000000000000000000"), currency::wide_difficulty_type("4712423267441696057"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1185000000000000000000000000"), currency::wide_difficulty_type("4670451020980647133"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1185000000000000000000000000"), currency::wide_difficulty_type("4670451020980647132"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1190000000000000000000000000"), currency::wide_difficulty_type("4629563291515467266"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1190000000000000000000000000"), currency::wide_difficulty_type("4629563291515467265"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1195000000000000000000000000"), currency::wide_difficulty_type("4589718581075682727"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1195000000000000000000000000"), currency::wide_difficulty_type("4589718581075682726"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1200000000000000000000000000"), currency::wide_difficulty_type("4550877482198821754"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1200000000000000000000000000"), currency::wide_difficulty_type("4550877482198821753"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1205000000000000000000000000"), currency::wide_difficulty_type("4513002547927988098"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1205000000000000000000000000"), currency::wide_difficulty_type("4513002547927988097"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1210000000000000000000000000"), currency::wide_difficulty_type("4476058171391610675"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1210000000000000000000000000"), currency::wide_difficulty_type("4476058171391610674"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1215000000000000000000000000"), currency::wide_difficulty_type("4440010474151375716"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1215000000000000000000000000"), currency::wide_difficulty_type("4440010474151375715"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1220000000000000000000000000"), currency::wide_difficulty_type("4404827202582426266"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1220000000000000000000000000"), currency::wide_difficulty_type("4404827202582426265"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1225000000000000000000000000"), currency::wide_difficulty_type("4370477631619666641"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1225000000000000000000000000"), currency::wide_difficulty_type("4370477631619666640"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1230000000000000000000000000"), currency::wide_difficulty_type("4336932475266411512"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1230000000000000000000000000"), currency::wide_difficulty_type("4336932475266411511"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1235000000000000000000000000"), currency::wide_difficulty_type("4304163803317521313"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1235000000000000000000000000"), currency::wide_difficulty_type("4304163803317521312"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1240000000000000000000000000"), currency::wide_difficulty_type("4272144963799311315"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1240000000000000000000000000"), currency::wide_difficulty_type("4272144963799311314"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1245000000000000000000000000"), currency::wide_difficulty_type("4240850510673562277"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1245000000000000000000000000"), currency::wide_difficulty_type("4240850510673562276"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1250000000000000000000000000"), currency::wide_difficulty_type("4210256136393466936"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1250000000000000000000000000"), currency::wide_difficulty_type("4210256136393466935"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1255000000000000000000000000"), currency::wide_difficulty_type("4180338608935820328"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1255000000000000000000000000"), currency::wide_difficulty_type("4180338608935820327"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1260000000000000000000000000"), currency::wide_difficulty_type("4151075712966644396"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1260000000000000000000000000"), currency::wide_difficulty_type("4151075712966644395"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1265000000000000000000000000"), currency::wide_difficulty_type("4122446194827116866"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1265000000000000000000000000"), currency::wide_difficulty_type("4122446194827116865"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1270000000000000000000000000"), currency::wide_difficulty_type("4094429711053494243"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1270000000000000000000000000"), currency::wide_difficulty_type("4094429711053494242"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1275000000000000000000000000"), currency::wide_difficulty_type("4067006780168981652"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1275000000000000000000000000"), currency::wide_difficulty_type("4067006780168981651"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1280000000000000000000000000"), currency::wide_difficulty_type("4040158737507476210"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1280000000000000000000000000"), currency::wide_difficulty_type("4040158737507476209"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1285000000000000000000000000"), currency::wide_difficulty_type("4013867692849033231"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1285000000000000000000000000"), currency::wide_difficulty_type("4013867692849033230"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1290000000000000000000000000"), currency::wide_difficulty_type("3988116490664986212"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1290000000000000000000000000"), currency::wide_difficulty_type("3988116490664986211"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1295000000000000000000000000"), currency::wide_difficulty_type("3962888672787079503"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1295000000000000000000000000"), currency::wide_difficulty_type("3962888672787079502"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1300000000000000000000000000"), currency::wide_difficulty_type("3938168443329913124"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1300000000000000000000000000"), currency::wide_difficulty_type("3938168443329913123"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1305000000000000000000000000"), currency::wide_difficulty_type("3913940635709600402"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1305000000000000000000000000"), currency::wide_difficulty_type("3913940635709600401"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1310000000000000000000000000"), currency::wide_difficulty_type("3890190681613933281"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1310000000000000000000000000"), currency::wide_difficulty_type("3890190681613933280"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1315000000000000000000000000"), currency::wide_difficulty_type("3866904581790654891"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1315000000000000000000000000"), currency::wide_difficulty_type("3866904581790654890"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1320000000000000000000000000"), currency::wide_difficulty_type("3844068878530759853"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1320000000000000000000000000"), currency::wide_difficulty_type("3844068878530759852"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1325000000000000000000000000"), currency::wide_difficulty_type("3821670629733173919"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1325000000000000000000000000"), currency::wide_difficulty_type("3821670629733173918"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1330000000000000000000000000"), currency::wide_difficulty_type("3799697384445790394"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1330000000000000000000000000"), currency::wide_difficulty_type("3799697384445790393"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1335000000000000000000000000"), currency::wide_difficulty_type("3778137159785737024"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1335000000000000000000000000"), currency::wide_difficulty_type("3778137159785737023"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1340000000000000000000000000"), currency::wide_difficulty_type("3756978419148981611"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1340000000000000000000000000"), currency::wide_difficulty_type("3756978419148981610"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1345000000000000000000000000"), currency::wide_difficulty_type("3736210051626018480"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1345000000000000000000000000"), currency::wide_difficulty_type("3736210051626018479"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1350000000000000000000000000"), currency::wide_difficulty_type("3715821352546466165"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1350000000000000000000000000"), currency::wide_difficulty_type("3715821352546466164"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1355000000000000000000000000"), currency::wide_difficulty_type("3695802005080998506"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1355000000000000000000000000"), currency::wide_difficulty_type("3695802005080998505"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1360000000000000000000000000"), currency::wide_difficulty_type("3676142062834171471"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1360000000000000000000000000"), currency::wide_difficulty_type("3676142062834171470"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1365000000000000000000000000"), currency::wide_difficulty_type("3656831933366436577"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1365000000000000000000000000"), currency::wide_difficulty_type("3656831933366436576"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1370000000000000000000000000"), currency::wide_difficulty_type("3637862362587985060"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1370000000000000000000000000"), currency::wide_difficulty_type("3637862362587985059"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1375000000000000000000000000"), currency::wide_difficulty_type("3619224419971077733"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1375000000000000000000000000"), currency::wide_difficulty_type("3619224419971077732"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1380000000000000000000000000"), currency::wide_difficulty_type("3600909484531213505"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1380000000000000000000000000"), currency::wide_difficulty_type("3600909484531213504"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1385000000000000000000000000"), currency::wide_difficulty_type("3582909231530901607"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1385000000000000000000000000"), currency::wide_difficulty_type("3582909231530901606"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1390000000000000000000000000"), currency::wide_difficulty_type("3565215619862953101"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1390000000000000000000000000"), currency::wide_difficulty_type("3565215619862953100"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1395000000000000000000000000"), currency::wide_difficulty_type("3547820880073118287"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1395000000000000000000000000"), currency::wide_difficulty_type("3547820880073118286"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1400000000000000000000000000"), currency::wide_difficulty_type("3530717502984588252"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1400000000000000000000000000"), currency::wide_difficulty_type("3530717502984588251"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1405000000000000000000000000"), currency::wide_difficulty_type("3513898228889369242"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1405000000000000000000000000"), currency::wide_difficulty_type("3513898228889369241"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1410000000000000000000000000"), currency::wide_difficulty_type("3497356037273844307"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1410000000000000000000000000"), currency::wide_difficulty_type("3497356037273844306"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1415000000000000000000000000"), currency::wide_difficulty_type("3481084137047972976"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1415000000000000000000000000"), currency::wide_difficulty_type("3481084137047972975"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1420000000000000000000000000"), currency::wide_difficulty_type("3465075957249560147"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1420000000000000000000000000"), currency::wide_difficulty_type("3465075957249560146"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1425000000000000000000000000"), currency::wide_difficulty_type("3449325138196862568"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1425000000000000000000000000"), currency::wide_difficulty_type("3449325138196862567"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1430000000000000000000000000"), currency::wide_difficulty_type("3433825523064506644"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1430000000000000000000000000"), currency::wide_difficulty_type("3433825523064506643"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1435000000000000000000000000"), currency::wide_difficulty_type("3418571149859275151"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1435000000000000000000000000"), currency::wide_difficulty_type("3418571149859275150"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1440000000000000000000000000"), currency::wide_difficulty_type("3403556243773792521"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1440000000000000000000000000"), currency::wide_difficulty_type("3403556243773792520"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1445000000000000000000000000"), currency::wide_difficulty_type("3388775209897507173"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1445000000000000000000000000"), currency::wide_difficulty_type("3388775209897507172"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1450000000000000000000000000"), currency::wide_difficulty_type("3374222626265643002"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1450000000000000000000000000"), currency::wide_difficulty_type("3374222626265643001"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1455000000000000000000000000"), currency::wide_difficulty_type("3359893237227977874"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1455000000000000000000000000"), currency::wide_difficulty_type("3359893237227977873"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1460000000000000000000000000"), currency::wide_difficulty_type("3345781947120411526"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1460000000000000000000000000"), currency::wide_difficulty_type("3345781947120411525"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1465000000000000000000000000"), currency::wide_difficulty_type("3331883814223314654"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1465000000000000000000000000"), currency::wide_difficulty_type("3331883814223314653"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1470000000000000000000000000"), currency::wide_difficulty_type("3318194044991611032"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1470000000000000000000000000"), currency::wide_difficulty_type("3318194044991611031"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1475000000000000000000000000"), currency::wide_difficulty_type("3304707988542440156"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1475000000000000000000000000"), currency::wide_difficulty_type("3304707988542440155"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1480000000000000000000000000"), currency::wide_difficulty_type("3291421131387084121"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1480000000000000000000000000"), currency::wide_difficulty_type("3291421131387084120"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1485000000000000000000000000"), currency::wide_difficulty_type("3278329092394623406"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1485000000000000000000000000"), currency::wide_difficulty_type("3278329092394623405"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1490000000000000000000000000"), currency::wide_difficulty_type("3265427617975516113"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1490000000000000000000000000"), currency::wide_difficulty_type("3265427617975516112"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1495000000000000000000000000"), currency::wide_difficulty_type("3252712577473977564"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1495000000000000000000000000"), currency::wide_difficulty_type("3252712577473977563"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1500000000000000000000000000"), currency::wide_difficulty_type("3240179958758675520"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1500000000000000000000000000"), currency::wide_difficulty_type("3240179958758675519"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1505000000000000000000000000"), currency::wide_difficulty_type("3227825864001853654"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1505000000000000000000000000"), currency::wide_difficulty_type("3227825864001853653"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1510000000000000000000000000"), currency::wide_difficulty_type("3215646505637555347"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1510000000000000000000000000"), currency::wide_difficulty_type("3215646505637555346"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1515000000000000000000000000"), currency::wide_difficulty_type("3203638202490143865"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1515000000000000000000000000"), currency::wide_difficulty_type("3203638202490143864"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1520000000000000000000000000"), currency::wide_difficulty_type("3191797376064806144"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1520000000000000000000000000"), currency::wide_difficulty_type("3191797376064806143"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1525000000000000000000000000"), currency::wide_difficulty_type("3180120546992187929"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1525000000000000000000000000"), currency::wide_difficulty_type("3180120546992187928"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1530000000000000000000000000"), currency::wide_difficulty_type("3168604331619740064"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1530000000000000000000000000"), currency::wide_difficulty_type("3168604331619740063"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1535000000000000000000000000"), currency::wide_difficulty_type("3157245438742761163"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1535000000000000000000000000"), currency::wide_difficulty_type("3157245438742761162"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1540000000000000000000000000"), currency::wide_difficulty_type("3146040666468502645"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1540000000000000000000000000"), currency::wide_difficulty_type("3146040666468502644"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1545000000000000000000000000"), currency::wide_difficulty_type("3134986899207059697"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1545000000000000000000000000"), currency::wide_difficulty_type("3134986899207059696"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1550000000000000000000000000"), currency::wide_difficulty_type("3124081104783107851"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1550000000000000000000000000"), currency::wide_difficulty_type("3124081104783107850"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1555000000000000000000000000"), currency::wide_difficulty_type("3113320331662860805"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1555000000000000000000000000"), currency::wide_difficulty_type("3113320331662860804"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1560000000000000000000000000"), currency::wide_difficulty_type("3102701706290922296"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1560000000000000000000000000"), currency::wide_difficulty_type("3102701706290922295"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1565000000000000000000000000"), currency::wide_difficulty_type("3092222430531984521"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1565000000000000000000000000"), currency::wide_difficulty_type("3092222430531984520"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1570000000000000000000000000"), currency::wide_difficulty_type("3081879779212588752"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1570000000000000000000000000"), currency::wide_difficulty_type("3081879779212588751"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1575000000000000000000000000"), currency::wide_difficulty_type("3071671097758411764"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1575000000000000000000000000"), currency::wide_difficulty_type("3071671097758411763"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1580000000000000000000000000"), currency::wide_difficulty_type("3061593799922775157"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1580000000000000000000000000"), currency::wide_difficulty_type("3061593799922775156"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1585000000000000000000000000"), currency::wide_difficulty_type("3051645365602294788"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1585000000000000000000000000"), currency::wide_difficulty_type("3051645365602294787"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1590000000000000000000000000"), currency::wide_difficulty_type("3041823338735795027"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1590000000000000000000000000"), currency::wide_difficulty_type("3041823338735795026"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1595000000000000000000000000"), currency::wide_difficulty_type("3032125325282808256"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1595000000000000000000000000"), currency::wide_difficulty_type("3032125325282808255"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1600000000000000000000000000"), currency::wide_difficulty_type("3022548991278164715"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1600000000000000000000000000"), currency::wide_difficulty_type("3022548991278164714"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1605000000000000000000000000"), currency::wide_difficulty_type("3013092060959352101"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1605000000000000000000000000"), currency::wide_difficulty_type("3013092060959352100"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1610000000000000000000000000"), currency::wide_difficulty_type("3003752314963488897"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1610000000000000000000000000"), currency::wide_difficulty_type("3003752314963488896"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1615000000000000000000000000"), currency::wide_difficulty_type("2994527588590910913"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1615000000000000000000000000"), currency::wide_difficulty_type("2994527588590910912"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1620000000000000000000000000"), currency::wide_difficulty_type("2985415770132517377"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1620000000000000000000000000"), currency::wide_difficulty_type("2985415770132517376"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1625000000000000000000000000"), currency::wide_difficulty_type("2976414799258161798"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1625000000000000000000000000"), currency::wide_difficulty_type("2976414799258161797"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1630000000000000000000000000"), currency::wide_difficulty_type("2967522665463504146"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1630000000000000000000000000"), currency::wide_difficulty_type("2967522665463504145"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1635000000000000000000000000"), currency::wide_difficulty_type("2958737406572865050"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1635000000000000000000000000"), currency::wide_difficulty_type("2958737406572865049"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1640000000000000000000000000"), currency::wide_difficulty_type("2950057107295740304"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1640000000000000000000000000"), currency::wide_difficulty_type("2950057107295740303"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1645000000000000000000000000"), currency::wide_difficulty_type("2941479897834745142"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1645000000000000000000000000"), currency::wide_difficulty_type("2941479897834745141"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1650000000000000000000000000"), currency::wide_difficulty_type("2933003952542863155"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1650000000000000000000000000"), currency::wide_difficulty_type("2933003952542863154"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1655000000000000000000000000"), currency::wide_difficulty_type("2924627488627974429"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1655000000000000000000000000"), currency::wide_difficulty_type("2924627488627974428"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1660000000000000000000000000"), currency::wide_difficulty_type("2916348764902732061"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1660000000000000000000000000"), currency::wide_difficulty_type("2916348764902732060"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1665000000000000000000000000"), currency::wide_difficulty_type("2908166080577945776"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1665000000000000000000000000"), currency::wide_difficulty_type("2908166080577945775"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1670000000000000000000000000"), currency::wide_difficulty_type("2900077774097716347"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1670000000000000000000000000"), currency::wide_difficulty_type("2900077774097716346"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1675000000000000000000000000"), currency::wide_difficulty_type("2892082222014645048"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1675000000000000000000000000"), currency::wide_difficulty_type("2892082222014645047"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1680000000000000000000000000"), currency::wide_difficulty_type("2884177837903518828"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1680000000000000000000000000"), currency::wide_difficulty_type("2884177837903518827"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1685000000000000000000000000"), currency::wide_difficulty_type("2876363071311944416"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1685000000000000000000000000"), currency::wide_difficulty_type("2876363071311944415"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1690000000000000000000000000"), currency::wide_difficulty_type("2868636406746473411"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1690000000000000000000000000"), currency::wide_difficulty_type("2868636406746473410"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1695000000000000000000000000"), currency::wide_difficulty_type("2860996362692825797"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1695000000000000000000000000"), currency::wide_difficulty_type("2860996362692825796"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1700000000000000000000000000"), currency::wide_difficulty_type("2853441490668881421"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1700000000000000000000000000"), currency::wide_difficulty_type("2853441490668881420"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1705000000000000000000000000"), currency::wide_difficulty_type("2845970374309167960"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1705000000000000000000000000"), currency::wide_difficulty_type("2845970374309167959"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1710000000000000000000000000"), currency::wide_difficulty_type("2838581628479629993"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1710000000000000000000000000"), currency::wide_difficulty_type("2838581628479629992"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1715000000000000000000000000"), currency::wide_difficulty_type("2831273898421517086"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1715000000000000000000000000"), currency::wide_difficulty_type("2831273898421517085"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1720000000000000000000000000"), currency::wide_difficulty_type("2824045858923279501"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1720000000000000000000000000"), currency::wide_difficulty_type("2824045858923279500"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1725000000000000000000000000"), currency::wide_difficulty_type("2816896213519408333"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1725000000000000000000000000"), currency::wide_difficulty_type("2816896213519408332"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1730000000000000000000000000"), currency::wide_difficulty_type("2809823693715202785"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1730000000000000000000000000"), currency::wide_difficulty_type("2809823693715202784"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1735000000000000000000000000"), currency::wide_difficulty_type("2802827058236490934"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1735000000000000000000000000"), currency::wide_difficulty_type("2802827058236490933"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1740000000000000000000000000"), currency::wide_difficulty_type("2795905092303371895"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1740000000000000000000000000"), currency::wide_difficulty_type("2795905092303371894"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1745000000000000000000000000"), currency::wide_difficulty_type("2789056606927086901"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1745000000000000000000000000"), currency::wide_difficulty_type("2789056606927086900"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1750000000000000000000000000"), currency::wide_difficulty_type("2782280438229164471"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1750000000000000000000000000"), currency::wide_difficulty_type("2782280438229164470"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1755000000000000000000000000"), currency::wide_difficulty_type("2775575446782020797"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1755000000000000000000000000"), currency::wide_difficulty_type("2775575446782020796"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1760000000000000000000000000"), currency::wide_difficulty_type("2768940516970230676"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1760000000000000000000000000"), currency::wide_difficulty_type("2768940516970230675"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1765000000000000000000000000"), currency::wide_difficulty_type("2762374556371716950"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1765000000000000000000000000"), currency::wide_difficulty_type("2762374556371716949"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1770000000000000000000000000"), currency::wide_difficulty_type("2755876495158137534"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1770000000000000000000000000"), currency::wide_difficulty_type("2755876495158137533"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1775000000000000000000000000"), currency::wide_difficulty_type("2749445285513778759"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1775000000000000000000000000"), currency::wide_difficulty_type("2749445285513778758"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1780000000000000000000000000"), currency::wide_difficulty_type("2743079901072292083"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1780000000000000000000000000"), currency::wide_difficulty_type("2743079901072292082"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1785000000000000000000000000"), currency::wide_difficulty_type("2736779336370638195"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1785000000000000000000000000"), currency::wide_difficulty_type("2736779336370638194"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1790000000000000000000000000"), currency::wide_difficulty_type("2730542606319628350"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1790000000000000000000000000"), currency::wide_difficulty_type("2730542606319628349"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1795000000000000000000000000"), currency::wide_difficulty_type("2724368745690477334"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1795000000000000000000000000"), currency::wide_difficulty_type("2724368745690477333"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1800000000000000000000000000"), currency::wide_difficulty_type("2718256808616805999"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1800000000000000000000000000"), currency::wide_difficulty_type("2718256808616805998"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1805000000000000000000000000"), currency::wide_difficulty_type("2712205868111553685"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1805000000000000000000000000"), currency::wide_difficulty_type("2712205868111553684"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1810000000000000000000000000"), currency::wide_difficulty_type("2706215015598282356"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1810000000000000000000000000"), currency::wide_difficulty_type("2706215015598282355"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1815000000000000000000000000"), currency::wide_difficulty_type("2700283360456374682"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1815000000000000000000000000"), currency::wide_difficulty_type("2700283360456374681"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1820000000000000000000000000"), currency::wide_difficulty_type("2694410029579647946"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1820000000000000000000000000"), currency::wide_difficulty_type("2694410029579647945"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1825000000000000000000000000"), currency::wide_difficulty_type("2688594166947924317"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1825000000000000000000000000"), currency::wide_difficulty_type("2688594166947924316"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1830000000000000000000000000"), currency::wide_difficulty_type("2682834933211115942"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1830000000000000000000000000"), currency::wide_difficulty_type("2682834933211115941"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1835000000000000000000000000"), currency::wide_difficulty_type("2677131505285400445"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1835000000000000000000000000"), currency::wide_difficulty_type("2677131505285400444"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1840000000000000000000000000"), currency::wide_difficulty_type("2671483075961078746"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1840000000000000000000000000"), currency::wide_difficulty_type("2671483075961078745"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1845000000000000000000000000"), currency::wide_difficulty_type("2665888853521722831"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1845000000000000000000000000"), currency::wide_difficulty_type("2665888853521722830"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1850000000000000000000000000"), currency::wide_difficulty_type("2660348061374236044"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1850000000000000000000000000"), currency::wide_difficulty_type("2660348061374236043"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1855000000000000000000000000"), currency::wide_difficulty_type("2654859937689462866"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1855000000000000000000000000"), currency::wide_difficulty_type("2654859937689462865"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1860000000000000000000000000"), currency::wide_difficulty_type("2649423735052998850"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1860000000000000000000000000"), currency::wide_difficulty_type("2649423735052998849"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1865000000000000000000000000"), currency::wide_difficulty_type("2644038720125864563"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1865000000000000000000000000"), currency::wide_difficulty_type("2644038720125864562"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1870000000000000000000000000"), currency::wide_difficulty_type("2638704173314719972"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1870000000000000000000000000"), currency::wide_difficulty_type("2638704173314719971"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1875000000000000000000000000"), currency::wide_difficulty_type("2633419388451307786"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1875000000000000000000000000"), currency::wide_difficulty_type("2633419388451307785"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1880000000000000000000000000"), currency::wide_difficulty_type("2628183672480825857"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1880000000000000000000000000"), currency::wide_difficulty_type("2628183672480825856"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1885000000000000000000000000"), currency::wide_difficulty_type("2622996345158939776"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1885000000000000000000000000"), currency::wide_difficulty_type("2622996345158939775"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1890000000000000000000000000"), currency::wide_difficulty_type("2617856738757157503"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1890000000000000000000000000"), currency::wide_difficulty_type("2617856738757157502"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1895000000000000000000000000"), currency::wide_difficulty_type("2612764197776297996"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1895000000000000000000000000"), currency::wide_difficulty_type("2612764197776297995"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1900000000000000000000000000"), currency::wide_difficulty_type("2607718078667795597"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1900000000000000000000000000"), currency::wide_difficulty_type("2607718078667795596"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1905000000000000000000000000"), currency::wide_difficulty_type("2602717749562591346"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1905000000000000000000000000"), currency::wide_difficulty_type("2602717749562591345"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1910000000000000000000000000"), currency::wide_difficulty_type("2597762590007371298"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1910000000000000000000000000"), currency::wide_difficulty_type("2597762590007371297"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1915000000000000000000000000"), currency::wide_difficulty_type("2592851990707920649"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1915000000000000000000000000"), currency::wide_difficulty_type("2592851990707920648"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1920000000000000000000000000"), currency::wide_difficulty_type("2587985353279370678"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1920000000000000000000000000"), currency::wide_difficulty_type("2587985353279370677"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1925000000000000000000000000"), currency::wide_difficulty_type("2583162090003123494"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1925000000000000000000000000"), currency::wide_difficulty_type("2583162090003123493"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1930000000000000000000000000"), currency::wide_difficulty_type("2578381623590247222"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1930000000000000000000000000"), currency::wide_difficulty_type("2578381623590247221"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1935000000000000000000000000"), currency::wide_difficulty_type("2573643386951141553"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1935000000000000000000000000"), currency::wide_difficulty_type("2573643386951141552"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1940000000000000000000000000"), currency::wide_difficulty_type("2568946822971280653"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1940000000000000000000000000"), currency::wide_difficulty_type("2568946822971280652"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1945000000000000000000000000"), currency::wide_difficulty_type("2564291384292847167"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1945000000000000000000000000"), currency::wide_difficulty_type("2564291384292847166"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1950000000000000000000000000"), currency::wide_difficulty_type("2559676533102077564"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1950000000000000000000000000"), currency::wide_difficulty_type("2559676533102077563"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1955000000000000000000000000"), currency::wide_difficulty_type("2555101740922145295"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1955000000000000000000000000"), currency::wide_difficulty_type("2555101740922145294"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1960000000000000000000000000"), currency::wide_difficulty_type("2550566488411414260"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1960000000000000000000000000"), currency::wide_difficulty_type("2550566488411414259"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1965000000000000000000000000"), currency::wide_difficulty_type("2546070265166900802"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1965000000000000000000000000"), currency::wide_difficulty_type("2546070265166900801"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1970000000000000000000000000"), currency::wide_difficulty_type("2541612569532788044"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1970000000000000000000000000"), currency::wide_difficulty_type("2541612569532788043"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1975000000000000000000000000"), currency::wide_difficulty_type("2537192908413841681"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1975000000000000000000000000"), currency::wide_difficulty_type("2537192908413841680"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1980000000000000000000000000"), currency::wide_difficulty_type("2532810797093581470"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1980000000000000000000000000"), currency::wide_difficulty_type("2532810797093581469"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1985000000000000000000000000"), currency::wide_difficulty_type("2528465759057067615"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1985000000000000000000000000"), currency::wide_difficulty_type("2528465759057067614"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1990000000000000000000000000"), currency::wide_difficulty_type("2524157325818165947"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1990000000000000000000000000"), currency::wide_difficulty_type("2524157325818165946"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1995000000000000000000000000"), currency::wide_difficulty_type("2519885036751160428"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("1995000000000000000000000000"), currency::wide_difficulty_type("2519885036751160427"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2000000000000000000000000000"), currency::wide_difficulty_type("2515648438926585831"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2000000000000000000000000000"), currency::wide_difficulty_type("2515648438926585830"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2005000000000000000000000000"), currency::wide_difficulty_type("2511447086951157734"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2005000000000000000000000000"), currency::wide_difficulty_type("2511447086951157733"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2010000000000000000000000000"), currency::wide_difficulty_type("2507280542811681016"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2010000000000000000000000000"), currency::wide_difficulty_type("2507280542811681015"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2015000000000000000000000000"), currency::wide_difficulty_type("2503148375722821955"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2015000000000000000000000000"), currency::wide_difficulty_type("2503148375722821954"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2020000000000000000000000000"), currency::wide_difficulty_type("2499050161978632830"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2020000000000000000000000000"), currency::wide_difficulty_type("2499050161978632829"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2025000000000000000000000000"), currency::wide_difficulty_type("2494985484807721531"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2025000000000000000000000000"), currency::wide_difficulty_type("2494985484807721530"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2030000000000000000000000000"), currency::wide_difficulty_type("2490953934231962225"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2030000000000000000000000000"), currency::wide_difficulty_type("2490953934231962224"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2035000000000000000000000000"), currency::wide_difficulty_type("2486955106928646474"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2035000000000000000000000000"), currency::wide_difficulty_type("2486955106928646473"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2040000000000000000000000000"), currency::wide_difficulty_type("2482988606095977448"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2040000000000000000000000000"), currency::wide_difficulty_type("2482988606095977447"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2045000000000000000000000000"), currency::wide_difficulty_type("2479054041321813044"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2045000000000000000000000000"), currency::wide_difficulty_type("2479054041321813043"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2050000000000000000000000000"), currency::wide_difficulty_type("2475151028455566714"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2050000000000000000000000000"), currency::wide_difficulty_type("2475151028455566713"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2055000000000000000000000000"), currency::wide_difficulty_type("2471279189483177729"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2055000000000000000000000000"), currency::wide_difficulty_type("2471279189483177728"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2060000000000000000000000000"), currency::wide_difficulty_type("2467438152405065410"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2060000000000000000000000000"), currency::wide_difficulty_type("2467438152405065409"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2065000000000000000000000000"), currency::wide_difficulty_type("2463627551116984585"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2065000000000000000000000000"), currency::wide_difficulty_type("2463627551116984584"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2070000000000000000000000000"), currency::wide_difficulty_type("2459847025293702108"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2070000000000000000000000000"), currency::wide_difficulty_type("2459847025293702107"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2075000000000000000000000000"), currency::wide_difficulty_type("2456096220275416833"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2075000000000000000000000000"), currency::wide_difficulty_type("2456096220275416832"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2080000000000000000000000000"), currency::wide_difficulty_type("2452374786956847850"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2080000000000000000000000000"), currency::wide_difficulty_type("2452374786956847849"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2085000000000000000000000000"), currency::wide_difficulty_type("2448682381678918120"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2085000000000000000000000000"), currency::wide_difficulty_type("2448682381678918119"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2090000000000000000000000000"), currency::wide_difficulty_type("2445018666122962957"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2090000000000000000000000000"), currency::wide_difficulty_type("2445018666122962956"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2095000000000000000000000000"), currency::wide_difficulty_type("2441383307207394935"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2095000000000000000000000000"), currency::wide_difficulty_type("2441383307207394934"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2100000000000000000000000000"), currency::wide_difficulty_type("2437775976986758940"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2100000000000000000000000000"), currency::wide_difficulty_type("2437775976986758939"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2105000000000000000000000000"), currency::wide_difficulty_type("2434196352553113130"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2105000000000000000000000000"), currency::wide_difficulty_type("2434196352553113129"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2110000000000000000000000000"), currency::wide_difficulty_type("2430644115939673490"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2110000000000000000000000000"), currency::wide_difficulty_type("2430644115939673489"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2115000000000000000000000000"), currency::wide_difficulty_type("2427118954026661635"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2115000000000000000000000000"), currency::wide_difficulty_type("2427118954026661634"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2120000000000000000000000000"), currency::wide_difficulty_type("2423620558449297271"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2120000000000000000000000000"), currency::wide_difficulty_type("2423620558449297270"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2125000000000000000000000000"), currency::wide_difficulty_type("2420148625507878558"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2125000000000000000000000000"), currency::wide_difficulty_type("2420148625507878557"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2130000000000000000000000000"), currency::wide_difficulty_type("2416702856079895306"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2130000000000000000000000000"), currency::wide_difficulty_type("2416702856079895305"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2135000000000000000000000000"), currency::wide_difficulty_type("2413282955534121575"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2135000000000000000000000000"), currency::wide_difficulty_type("2413282955534121574"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2140000000000000000000000000"), currency::wide_difficulty_type("2409888633646635875"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2140000000000000000000000000"), currency::wide_difficulty_type("2409888633646635874"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2145000000000000000000000000"), currency::wide_difficulty_type("2406519604518718694"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2145000000000000000000000000"), currency::wide_difficulty_type("2406519604518718693"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2150000000000000000000000000"), currency::wide_difficulty_type("2403175586496578565"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2150000000000000000000000000"), currency::wide_difficulty_type("2403175586496578564"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2155000000000000000000000000"), currency::wide_difficulty_type("2399856302092859343"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2155000000000000000000000000"), currency::wide_difficulty_type("2399856302092859342"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2160000000000000000000000000"), currency::wide_difficulty_type("2396561477909882754"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2160000000000000000000000000"), currency::wide_difficulty_type("2396561477909882753"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2165000000000000000000000000"), currency::wide_difficulty_type("2393290844564581615"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2165000000000000000000000000"), currency::wide_difficulty_type("2393290844564581614"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2170000000000000000000000000"), currency::wide_difficulty_type("2390044136615080450"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2170000000000000000000000000"), currency::wide_difficulty_type("2390044136615080449"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2175000000000000000000000000"), currency::wide_difficulty_type("2386821092488881460"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2175000000000000000000000000"), currency::wide_difficulty_type("2386821092488881459"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2180000000000000000000000000"), currency::wide_difficulty_type("2383621454412615068"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2180000000000000000000000000"), currency::wide_difficulty_type("2383621454412615067"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2185000000000000000000000000"), currency::wide_difficulty_type("2380444968343315390"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2185000000000000000000000000"), currency::wide_difficulty_type("2380444968343315389"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2190000000000000000000000000"), currency::wide_difficulty_type("2377291383901182163"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2190000000000000000000000000"), currency::wide_difficulty_type("2377291383901182162"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2195000000000000000000000000"), currency::wide_difficulty_type("2374160454303791767"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2195000000000000000000000000"), currency::wide_difficulty_type("2374160454303791766"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2200000000000000000000000000"), currency::wide_difficulty_type("2371051936301721016"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2200000000000000000000000000"), currency::wide_difficulty_type("2371051936301721015"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2205000000000000000000000000"), currency::wide_difficulty_type("2367965590115548453"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2205000000000000000000000000"), currency::wide_difficulty_type("2367965590115548452"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2210000000000000000000000000"), currency::wide_difficulty_type("2364901179374198904"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2210000000000000000000000000"), currency::wide_difficulty_type("2364901179374198903"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2215000000000000000000000000"), currency::wide_difficulty_type("2361858471054597963"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2215000000000000000000000000"), currency::wide_difficulty_type("2361858471054597962"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2220000000000000000000000000"), currency::wide_difficulty_type("2358837235422604080"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2220000000000000000000000000"), currency::wide_difficulty_type("2358837235422604079"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2225000000000000000000000000"), currency::wide_difficulty_type("2355837245975186797"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2225000000000000000000000000"), currency::wide_difficulty_type("2355837245975186796"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2230000000000000000000000000"), currency::wide_difficulty_type("2352858279383820553"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2230000000000000000000000000"), currency::wide_difficulty_type("2352858279383820552"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2235000000000000000000000000"), currency::wide_difficulty_type("2349900115439064383"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2235000000000000000000000000"), currency::wide_difficulty_type("2349900115439064382"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2240000000000000000000000000"), currency::wide_difficulty_type("2346962536996298585"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2240000000000000000000000000"), currency::wide_difficulty_type("2346962536996298584"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2245000000000000000000000000"), currency::wide_difficulty_type("2344045329922590311"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2245000000000000000000000000"), currency::wide_difficulty_type("2344045329922590310"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2250000000000000000000000000"), currency::wide_difficulty_type("2341148283044660753"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2250000000000000000000000000"), currency::wide_difficulty_type("2341148283044660752"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2255000000000000000000000000"), currency::wide_difficulty_type("2338271188097927374"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2255000000000000000000000000"), currency::wide_difficulty_type("2338271188097927373"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2260000000000000000000000000"), currency::wide_difficulty_type("2335413839676595372"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2260000000000000000000000000"), currency::wide_difficulty_type("2335413839676595371"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2265000000000000000000000000"), currency::wide_difficulty_type("2332576035184773241"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2265000000000000000000000000"), currency::wide_difficulty_type("2332576035184773240"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2270000000000000000000000000"), currency::wide_difficulty_type("2329757574788588008"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2270000000000000000000000000"), currency::wide_difficulty_type("2329757574788588007"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2275000000000000000000000000"), currency::wide_difficulty_type("2326958261369276374"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2275000000000000000000000000"), currency::wide_difficulty_type("2326958261369276373"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2280000000000000000000000000"), currency::wide_difficulty_type("2324177900477228635"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2280000000000000000000000000"), currency::wide_difficulty_type("2324177900477228634"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2285000000000000000000000000"), currency::wide_difficulty_type("2321416300286962874"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2285000000000000000000000000"), currency::wide_difficulty_type("2321416300286962873"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2290000000000000000000000000"), currency::wide_difficulty_type("2318673271553007540"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2290000000000000000000000000"), currency::wide_difficulty_type("2318673271553007539"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2295000000000000000000000000"), currency::wide_difficulty_type("2315948627566671091"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2295000000000000000000000000"), currency::wide_difficulty_type("2315948627566671090"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2300000000000000000000000000"), currency::wide_difficulty_type("2313242184113677981"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2300000000000000000000000000"), currency::wide_difficulty_type("2313242184113677980"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2305000000000000000000000000"), currency::wide_difficulty_type("2310553759432650776"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2305000000000000000000000000"), currency::wide_difficulty_type("2310553759432650775"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2310000000000000000000000000"), currency::wide_difficulty_type("2307883174174418780"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2310000000000000000000000000"), currency::wide_difficulty_type("2307883174174418779"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2315000000000000000000000000"), currency::wide_difficulty_type("2305230251362134008"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2315000000000000000000000000"), currency::wide_difficulty_type("2305230251362134007"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2320000000000000000000000000"), currency::wide_difficulty_type("2302594816352175903"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2320000000000000000000000000"), currency::wide_difficulty_type("2302594816352175902"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2325000000000000000000000000"), currency::wide_difficulty_type("2299976696795826646"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2325000000000000000000000000"), currency::wide_difficulty_type("2299976696795826645"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2330000000000000000000000000"), currency::wide_difficulty_type("2297375722601699401"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2330000000000000000000000000"), currency::wide_difficulty_type("2297375722601699400"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2335000000000000000000000000"), currency::wide_difficulty_type("2294791725898902291"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2335000000000000000000000000"), currency::wide_difficulty_type("2294791725898902290"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2340000000000000000000000000"), currency::wide_difficulty_type("2292224541000921359"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2340000000000000000000000000"), currency::wide_difficulty_type("2292224541000921358"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2345000000000000000000000000"), currency::wide_difficulty_type("2289674004370206180"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2345000000000000000000000000"), currency::wide_difficulty_type("2289674004370206179"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2350000000000000000000000000"), currency::wide_difficulty_type("2287139954583442248"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2350000000000000000000000000"), currency::wide_difficulty_type("2287139954583442247"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2355000000000000000000000000"), currency::wide_difficulty_type("2284622232297494626"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2355000000000000000000000000"), currency::wide_difficulty_type("2284622232297494625"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2360000000000000000000000000"), currency::wide_difficulty_type("2282120680216007790"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2360000000000000000000000000"), currency::wide_difficulty_type("2282120680216007789"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2365000000000000000000000000"), currency::wide_difficulty_type("2279635143056646954"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2365000000000000000000000000"), currency::wide_difficulty_type("2279635143056646953"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2370000000000000000000000000"), currency::wide_difficulty_type("2277165467518966551"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2370000000000000000000000000"), currency::wide_difficulty_type("2277165467518966550"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2375000000000000000000000000"), currency::wide_difficulty_type("2274711502252891900"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2375000000000000000000000000"), currency::wide_difficulty_type("2274711502252891899"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2380000000000000000000000000"), currency::wide_difficulty_type("2272273097827800467"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2380000000000000000000000000"), currency::wide_difficulty_type("2272273097827800466"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2385000000000000000000000000"), currency::wide_difficulty_type("2269850106702189431"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2385000000000000000000000000"), currency::wide_difficulty_type("2269850106702189430"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2390000000000000000000000000"), currency::wide_difficulty_type("2267442383193916647"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2390000000000000000000000000"), currency::wide_difficulty_type("2267442383193916646"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2395000000000000000000000000"), currency::wide_difficulty_type("2265049783451002382"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2395000000000000000000000000"), currency::wide_difficulty_type("2265049783451002381"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2400000000000000000000000000"), currency::wide_difficulty_type("2262672165422979551"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2400000000000000000000000000"), currency::wide_difficulty_type("2262672165422979550"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2405000000000000000000000000"), currency::wide_difficulty_type("2260309388832780454"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2405000000000000000000000000"), currency::wide_difficulty_type("2260309388832780453"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2410000000000000000000000000"), currency::wide_difficulty_type("2257961315149148341"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2410000000000000000000000000"), currency::wide_difficulty_type("2257961315149148340"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2415000000000000000000000000"), currency::wide_difficulty_type("2255627807559562398"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2415000000000000000000000000"), currency::wide_difficulty_type("2255627807559562397"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2420000000000000000000000000"), currency::wide_difficulty_type("2253308730943665060"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2420000000000000000000000000"), currency::wide_difficulty_type("2253308730943665059"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2425000000000000000000000000"), currency::wide_difficulty_type("2251003951847180788"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2425000000000000000000000000"), currency::wide_difficulty_type("2251003951847180787"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2430000000000000000000000000"), currency::wide_difficulty_type("2248713338456315756"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2430000000000000000000000000"), currency::wide_difficulty_type("2248713338456315755"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2435000000000000000000000000"), currency::wide_difficulty_type("2246436760572628130"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2435000000000000000000000000"), currency::wide_difficulty_type("2246436760572628129"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2440000000000000000000000000"), currency::wide_difficulty_type("2244174089588358868"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2440000000000000000000000000"), currency::wide_difficulty_type("2244174089588358867"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2445000000000000000000000000"), currency::wide_difficulty_type("2241925198462213242"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2445000000000000000000000000"), currency::wide_difficulty_type("2241925198462213241"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2450000000000000000000000000"), currency::wide_difficulty_type("2239689961695583486"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2450000000000000000000000000"), currency::wide_difficulty_type("2239689961695583485"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2455000000000000000000000000"), currency::wide_difficulty_type("2237468255309203239"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2455000000000000000000000000"), currency::wide_difficulty_type("2237468255309203238"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2460000000000000000000000000"), currency::wide_difficulty_type("2235259956820224654"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2460000000000000000000000000"), currency::wide_difficulty_type("2235259956820224653"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2465000000000000000000000000"), currency::wide_difficulty_type("2233064945219709271"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2465000000000000000000000000"), currency::wide_difficulty_type("2233064945219709270"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2470000000000000000000000000"), currency::wide_difficulty_type("2230883100950523974"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2470000000000000000000000000"), currency::wide_difficulty_type("2230883100950523973"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2475000000000000000000000000"), currency::wide_difficulty_type("2228714305885633552"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2475000000000000000000000000"), currency::wide_difficulty_type("2228714305885633551"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2480000000000000000000000000"), currency::wide_difficulty_type("2226558443306781569"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2480000000000000000000000000"), currency::wide_difficulty_type("2226558443306781568"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2485000000000000000000000000"), currency::wide_difficulty_type("2224415397883551498"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2485000000000000000000000000"), currency::wide_difficulty_type("2224415397883551497"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2490000000000000000000000000"), currency::wide_difficulty_type("2222285055652800188"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2490000000000000000000000000"), currency::wide_difficulty_type("2222285055652800187"));
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2495000000000000000000000000"), currency::wide_difficulty_type("2220167303998455999"));
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(currency::wide_difficulty_type("2495000000000000000000000000"), currency::wide_difficulty_type("2220167303998455998"));
  ASSERT_FALSE(res);

 }
