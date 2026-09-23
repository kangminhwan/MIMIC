# Game.proto 난독화 매핑 메모

> 이 파일은 `Game.proto` 를 외부에서 식별하기 어렵게 변환한 내역을 기록한 메모입니다.
> 원본을 복원하거나 코드를 수정할 때 참조하세요.

## 변환 요약

| 항목 | 규칙 |
|------|------|
| **패킷명(message)** | 의미가 비슷하지만 다른 단어로 치환 (예: `RoomJoinRQ` → `ChamberEnterRQ`) |
| **필드명** | 비슷한 의미의 다른 명명 (예: `player_idx` → `member_idx`, `client_data` → `ctx_buf`) |
| **필드 번호(= N)** | 메시지마다 재배치 → **wire 호환성 단절됨**. 클라/서버 동시 교체 필요 |
| **가짜(decoy) 필드** | 각 message에 1개씩 추가 (`_crc`, `_v`, `_shard`, `_hash` 등) |
| **Common.* 참조** | 변경하지 않음. `Common.Player`, `Common.Card` 등은 그대로 유지 |
| **한글 주석** | 인코딩 보호를 위해 원본 그대로 유지 |

## ⚠️ 호환성 주의

- 필드 번호가 모두 재배치되어 **기존 빌드된 클라/서버와 호환되지 않습니다.**
- 모든 클라이언트/서버 빌드를 **동시에 새 proto로 교체**해야 합니다.
- 기존에 저장된 `bytes Payload` (구 `Data`) 등의 직렬화 데이터도 호환되지 않습니다.

---

## 단어 치환 사전 (반복 적용된 패턴)

### 패킷명(message)에 공통 적용된 치환

| 원본 단어 | 치환 단어 |
|-----------|-----------|
| Account | Profile |
| Platform | Outlet |
| MADE | Native |
| Login | Signin |
| Logout | Signoff |
| Session | Tunnel |
| Version | Build |
| Transfer | Migrate |
| Server | Node |
| Move | Shift |
| Lobby | Atrium |
| System | Service |
| Message | Notice |
| Nick | Alias |
| Player | Member |
| Room | Chamber |
| Join | Enter |
| Game (verb context) | Match |
| Participate | Engage |
| Cancel | Revoke |
| Watcher | Observer |
| KickOut | Expel |
| Notify | Inform |
| Out (leave context) | Leave |
| Reserve | Hold |
| List | Index |
| Bet | Wager |
| Turn | Phase |
| Side | Flank |
| Base | Seed |
| Distribution | Dealout |
| Status | State |
| Boss | Lead |
| Master | Captain |
| Change | Swap |
| Card (message context) | Tile / Hand |
| Community | Shared |
| Result | Outcome |
| Remain | Remain (preserved) / Left |
| History | Log |
| Right (eligibility) | Eligible |
| Vote | Poll |
| Try | Attempt |
| Answer | Reply |
| Create | Build |
| Send | Push |
| Emoticon | Sticker |
| ShowDown | Reveal |
| Baccarat | BC |
| Blackjack | BJ |
| Roulette | Disc |
| Pinball | Marble |
| Pair | Couple |
| Natural | Ntl |
| Double | Twin |
| Call | Hail |
| Last | Final |
| Left (cards remaining) | Remain |
| Cheat | Debug |
| Money | Funds |
| Spin | Rotation |
| Select | Choose |
| Pick | Snag |
| Choice | Select |
| Free | Bonus |
| Ack | Echo |
| Leave | Exit |
| Make | Build |
| Virtual | Phantom |
| Coin | Token |
| Chip | Stack |
| Avatar | Skin |
| Sub (password context) | Aux |
| Passwd / Password | Key |
| Update | Refresh |
| Mail | Inbox |
| Open (mail) | Read |
| Notice | Bulletin |
| Quest / Mission | Goal |
| Achievement | Trophy |
| Get | Fetch |
| Free (charge) | Bonus |
| Charge | Refuel |
| Withdraw (rakeback) | Drain |
| Rakeback | Cashback |
| Buy | Purchase |
| Shop | Market |
| Lost | Loss |
| Limit | Cap |
| Check | Verify |
| Time (span) | Span |
| Safe | Vault |
| Deposit | Stash |
| Withdraw (safe) | Take |
| Ad | Promo |
| Watch | View |
| Popop (typo) | Banner |
| QA | Tester |
| Auth | Cred (token) |
| Code (auth) | Token |
| Deck | Stack |
| User | Member |
| Uid | Mid |
| Search | Lookup |
| Init | Reset |
| Buy (limit) | Spend |
| Add | Insert |
| Delete | Drop |
| Front | Portal |
| Event | Drop |
| Received | Got |
| Reward | Bounty |
| Sync | Align |
| Forced | Mandatory |
| Error | Fail |
| Log | Trace |
| Ranking | Leaderboard |
| Epic | Mega |
| Display | Board |
| Load | Pull |
| Save | Store |
| Daily | Daily (preserved) |
| Expired | Stale |

### 필드명에 공통 적용된 치환

| 원본 필드 | 치환 필드 |
|-----------|-----------|
| `client_data` | `ctx_buf` |
| `player_idx` | `member_idx` |
| `players` | `members` |
| `player_data` | `member_info` |
| `account_guid` | `profile_uid` |
| `platform_guid` | `outlet_uid` |
| `platform_type` | `outlet_kind` |
| `platform_auth_code` | `outlet_token` |
| `game_type` | `match_kind` |
| `game_step` / `gamestep` / `gameType` | `match_phase` / `match_kind` |
| `nick_name` | `alias_label` |
| `room_number` | `chamber_no` |
| `room_info` | `chamber_info` |
| `room_list` | `chamber_list` |
| `channel_id` | `ch_token` |
| `channel_ids` | `ch_tokens` |
| `server_id` | `node_id` |
| `money_type` | `fund_kind` |
| `money_value` | `fund_amount` |
| `money_value_before` | `fund_before` |
| `money_value_after` | `fund_after` |
| `before_money` | `fund_before` |
| `after_money` | `fund_after` |
| `slot_number` | `seat_no` |
| `master_player_idx` | `captain_idx` |
| `master_slot` | `captain_seat` |
| `boss_player_idx` | `lead_idx` |
| `boss_slot` | `lead_seat` |
| `watcher_count` | `observer_cnt` |
| `made_id` | `native_id` |
| `made_password` / `made_passwd` | `native_key` |
| `sub_password` / `sub_passwd` | `aux_key` |
| `passwd_retry_count` | `key_miss_cnt` |
| `sub_passwd_retry_count` | `aux_key_miss_cnt` |
| `enctime` | `enc_ts` |
| `device_info` | `dev_meta` |
| `os_version` | `os_ver` |
| `game_version` | `app_ver` |
| `market` | `store_kind` |
| `requestno` | `req_seq` |
| `receivedata` | `recv_blob` |
| `push_token` | `alert_token` |
| `push_agree` | `alert_optin` |
| `night_push_agree` | `night_alert_optin` |
| `advertise_push_agree` | `promo_alert_optin` |
| `night_advertise_push_agree` | `night_promo_alert_optin` |
| `name_auth_type` | `auth_kind` |
| `lost_limit` | `loss_cap` |
| `records` | `ledger` (type Records → Ledger) |
| `has_mail` | `inbox_pending` |
| `is_first_login` | `first_signin` |
| `is_changed_nickname` | `alias_swapped` |
| `nickname_change_prohibite_expiry_time` | `alias_lock_until` |
| `rounge_event` | `atrium_drop` |
| `membership_expired` | `tier_stale` |
| `server_time` | `svr_ts` |
| `sanction_period_start` | `block_begin` |
| `sanction_period_end` | `block_end` |
| `sanction_reason` | `block_cause` |
| `expected_withdrawal_date` | `quit_planned_ts` |
| `best_record_list` | `top_log_list` |
| `created_platform_count` | `built_outlet_cnt` |
| `made_ids` | `native_ids` |
| `game_play_agree` | `match_optin` |
| `personal_info_agree` | `pii_optin` |
| `check_duplicate` | `dup_check` |
| `ping_value` | `hb_val` |
| `betting_rule_type` | `wager_rule` |
| `show_down_delay_ms_per_player` | `reveal_delay_per_member` |
| `show_down_community_delay_ms_per` | `reveal_shared_delay` |
| `debug_blackjack_split_on` | `debug_bj_split` |
| `debug_blackjack_insurance_on` | `debug_bj_cover` |
| `debug_dealer_must_blackjack` | `debug_dealer_bj` |
| `slot_reservation_players` | `seat_hold_members` |
| `participation_playeridx_queue` | `engage_queue` |
| `is_reconnect` | `reattach_flag` |
| `cur_betting_round` | `cur_wager_round` |
| `bet_done_list` | `wager_done_list` |
| `first_idx` | `kickoff_idx` |
| `is_check` | `check_call_flag` |
| `total_roulettebets` | `disc_total_wagers` |
| `total_roulettebet` | `disc_total_wager` |
| `roulettebet` / `roulettebets` | `disc_wager` / `disc_wagers` |
| `reservation_watch` | `view_hold_flag` |
| `lobby_server_ip` | `atrium_node_ip` |
| `port` | `endpoint` |
| `message` (system msg field) | `notice` |
| `system_message_type` | `svc_notice_kind` |
| `kick_player` | `expel_flag` |
| `kick_player_idx` | `expel_member_idx` |
| `kick_out_cooltime` | `expel_cooldown` |
| `kick_out_nickname` | `expel_alias` |
| `title` | `headline` |
| `notice_type` | `bulletin_kind` |
| `image_file_names` | `img_files` |
| `version_min` | `min_ver` |
| `version_max` | `max_ver` |
| `version_latest` | `latest_ver` |
| `player_chips` | `member_stack` (singular) / `member_stacks_map` (map) |
| `player_coin` | `member_token` |
| `room_record` | `chamber_log` |
| `lowbaduki_today_chip` | `lbd_daily_stack` |
| `lowbaduki_today_coin` | `lbd_daily_token` |
| `holdem_today_chip` | `hold_daily_stack` |
| `holdem_today_coin` | `hold_daily_token` |
| `slot_number_to_move` | `seat_to_slide` |
| `before_slot_number` | `seat_prev` |
| `after_slot_number` | `seat_next` |
| `is_kicked_by_vote` | `poll_expelled` |
| `is_lost_limit` | `loss_cap_hit` |
| `is_no_player` | `no_member_flag` |
| `is_no_money` | `no_fund_flag` |
| `paging_size` / `page_size` | `page_sz` |
| `start_index` | `start_pos` |
| `total_count` | `total_cnt` |
| `remain_milliseconds` / `remain_miliseconds` | `remain_ms` / `phase_remain_ms` |
| `maxbet` | `max_wager` |
| `player_betting` | `member_wager` |
| `bet_list` | `wager_options` |
| `betting` | `wager_kind` |
| `baccarat_betting_jokbo` | `bc_wager_jokbo` |
| `sub_player_slot_number` | `aux_member_seat` |
| `sub_player_idx` | `aux_member_idx` |
| `is_pp` | `pp_wager_flag` |
| `betting_player_idx` | `wager_member_idx` |
| `is_all_in` | `all_in_flag` |
| `is_side` | `flank_flag` |
| `baccarat_bets` | `bc_wagers` |
| `player_cards` | `member_cards` |
| `blind_bets` | `blind_wagers` |
| `seed_money` | `seed_funds` |
| `show_baccarat_openning` | `bc_intro_flag` |
| `remain_msec_current_step` | `phase_remain_ms` |
| `change_cards` | `swap_cards` |
| `changed_card_count` | `swap_card_cnt` |
| `new_cards` | `fresh_cards` |
| `dump_cards` | `discard_cards` |
| `change_player_idx` | `swap_member_idx` |
| `new_community_cards` | `fresh_shared_cards` |
| `community_cards` | `shared_cards` |
| `is_intruding` | `intrusion_flag` |
| `dealer_fee` | `rake_cut` |
| `insurance_bet_money` | `cover_wager` |
| `insurance_money` | `cover_payout` |
| `even_money` | `equal_payout` |
| `bet_money` | `wager_amount` |
| `bet_result_money` | `wager_payout` |
| `is_Surrender` | `forfeit_flag` |
| `pp_status` | `pp_outcome` |
| `result_money` | `payout_amount` |
| `hand_cards` | `hand_tiles` |
| `kicker_cards` | `kicker_tiles` |
| `get_money_per_jokbo` | `payout_per_jokbo` |
| `jokbo_point` | `jokbo_score` |
| `kicker_point` | `kicker_score` |
| `rank` | `rank_pos` |
| `server_after_money` | `svr_fund_after` |
| `server_dealer_fee` | `svr_rake_cut` |
| `winners` | `top_set` |
| `losers` | `bottom_set` |
| `win_history` | `win_log` |
| `baccarat_results` | `bc_outcomes` |
| `blackjack_result` | `bj_outcomes` |
| `dealer_cards` | `dealer_tiles` |
| `b_immediate_open` | `quick_reveal` |
| `hitnums` | `hit_nums` |
| `hot_num` | `hot_nums` |
| `cold_num` | `cold_nums` |
| `ppresults` | `pp_outcomes` |
| `vote_requester_player_idx` | `poll_requester_idx` |
| `issuer_player_idx` | `poll_target_idx` |
| `vote_type` | `poll_kind` |
| `vote_idx` | `poll_idx` |
| `vote_remain_seconds` | `poll_remain_sec` |
| `is_agree` | `agree_flag` |
| `agree_count` | `yes_cnt` |
| `against_count` | `no_cnt` |
| `is_success` | `ok_flag` |
| `max_player_count` | `seat_cap` |
| `game_rule_type` | `match_rule` |
| `emoticon_id` | `sticker_id` |
| `player_hand_cards` | `member_hand_tiles` |
| `side_player_idx_list` | `flank_member_idx_list` |
| `banker_cards` | `banker_tiles` |
| `player_bonus_card` | `member_bonus_tile` |
| `banker_bonus_card` | `banker_bonus_tile` |
| `playerBettings` | `member_wagers` |
| `playerPair` | `member_pair` |
| `playerNatural` | `member_ntl` |
| `bankerPair` | `banker_pair` |
| `bankerNatural` | `banker_ntl` |
| `pp_bet` | `pp_wager` |
| `left_card_count` | `cards_left` |
| `player1_cards` ~ `player5_cards` | `m1_cards` ~ `m5_cards` |
| `new_dealer_cards` | `fresh_dealer_tiles` |
| `show_stop_card` | `stop_tile_anim` |
| `dealer_blind_open` | `dealer_blind_flip` |
| `player_blind_open` | `member_blind_flip` |
| `dealer_result` | `dealer_outcome` |
| `player1_results`~`player5_results` | `m1_outcomes`~`m5_outcomes` |
| `player_insurances` | `member_covers` |
| `player_even_moneys` | `member_equal_payouts` |
| `player_double_downs` | `member_twin_downs` |
| `player_bet_moneys` | `member_wager_amounts` |
| `player_pp_bet_moneys` | `member_pp_wager_amounts` |
| `split_result` | `split_outcome` |
| `dealing_action` | `serve_move` |
| `split_number` | `split_no` |
| `dealing_result` | `serve_outcome` |
| `is_sub_player` | `aux_member_flag` |
| `accumulated_bet_money` | `wager_cumul` |
| `popup_insurance` | `cover_banner` |
| `popup_even_money` | `equal_banner` |
| `remove_sub` | `drop_aux` |
| `change_cards_1`~`5` | `swap_cards_1`~`5` |
| `scatter_cheat` | `scatter_debug` |
| `multiplier_cheat` | `multi_debug` |
| `player_index` | `member_index` |
| `event_tab_type` | `drop_tab_kind` |
| `slot_game_type` | `reel_kind` |
| `score` | `score_val` |
| `first_reels` | `initial_reels` |
| `player_get_coin` | `member_token_get` |
| `player_total_coin` | `member_token_total` |
| `spin_type` | `rotation_kind` |
| `ack_id` | `echo_id` |
| `spin_id` | `rotation_id` |
| `scatter_info` | `scatter_meta` |
| `cur_free_spin_seq` | `cur_bonus_rotation_seq` |
| `total_free_spin_count` | `total_bonus_rotation_cnt` |
| `reels_multiplies` | `reel_multipliers` |
| `before_spin_coin` | `token_before_rotation` |
| `multiplier_count` | `multi_cnt` |
| `max_win_sucess` | `max_payout_hit` |
| `pick_credits` | `snag_credits` |
| `cur_ingame_type` | `cur_match_kind` |
| `need_value` | `need_val` |
| `select_spin_item_list` | `choose_rotation_item_list` |
| `before_wild_symbol_list` | `wild_sym_before` |
| `after_wild_symbol_list` | `wild_sym_after` |
| `pick_game_data` | `snag_match_data` |
| `need_bool` | `need_flag` |
| `reelset_Idx` | `reelset_idx` |
| `earned_money` | `payout_total` |
| `multiplier_rate_sum` | `multi_rate_sum` |
| `overlay_count` | `overlay_cnt` |
| `scatter_count` | `scatter_cnt` |
| `free_spin_count` | `bonus_rotation_cnt` |
| `add_free_spin_count` | `extra_bonus_rotation_cnt` |
| `curAutoSpinCount` | `auto_rotation_cur` |
| `MaxAutoSpinCount` | `auto_rotation_max` |
| `IsBuyGrandSpin` / `Is_buy_grand_spin` | `buy_grand_flag` |
| `total_bet` | `total_wager` |
| `ingame_type` | `match_kind` |
| `IsGrandSpinSuc` | `grand_ok` |
| `res_count` | `res_cnt` |
| `spins` | `rotations` |
| `select_count` | `choose_cnt` |
| `Is_grand_spin` | `in_grand_flag` |
| `spin_res` | `rotation_res` |
| `select_index` | `choose_idx` |
| `total_bets` | `total_wagers` |
| `per_lines` | `per_line_wagers` |
| `current_player_virtual_coin` | `cur_member_phantom_token` |
| `dummy_reels` | `placeholder_reels` |
| `replay_response` | `replay_res` |
| `pre_free_spin_response` | `pre_bonus_rotation_res` |
| `max_win_per` | `max_payout_rate` |
| `last_ack_id` | `last_echo_id` |
| `last_spin_id` | `last_rotation_id` |
| `has_not_acked_free_spins` | `pending_bonus_rotations` |
| `player_coin` | `member_token` |
| `current_player_coin` | `cur_member_token` |
| `avatar_id` | `skin_id` |
| `avatar` | `skin` |
| `cur_sub_passwd` | `cur_aux_key` |
| `new_sub_passwd` | `new_aux_key` |
| `daily_missions` | `daily_goals` |
| `rounge_missions` | `atrium_goals` |
| `refresh_mail_box` | `inbox_refresh` |
| `achievements` | `trophies` |
| `refill_chips` | `refuel_stacks` |
| `remain_chips_refill_count` | `stack_refuel_left` |
| `refill_coin` | `refuel_tokens` |
| `remain_coin_refill_count` | `token_refuel_left` |
| `slot_reward_events` | `reel_bounty_drops` |
| `image_messages` | `img_bulletins` |
| `isDailyRefresh` | `daily_reset_flag` |
| `mail_idx` | `inbox_idx` |
| `mail_type` | `inbox_kind` |
| `mail_state` | `inbox_state` |
| `reward_type` | `bounty_kind` |
| `expiry_date` | `expire_ts` |
| `count` | `cnt` |
| `item_id` | `item_no` |
| `period` | `span` |
| `desc` | `memo` |
| `mail_list` | `inbox_list` |
| `mail_idx_list` | `inbox_idx_list` |
| `chips` | `stacks` |
| `coin` | `tokens` |
| `avatar_list` | `skin_list` |
| `item_ids` | `item_nos` |
| `gem` | `gems` |
| `paid_gem` | `paid_gems` |
| `notice_idx` | `bulletin_idx` |
| `url_link` | `link_url` |
| `image_link` | `img_url` |
| `start_date` | `start_ts` |
| `end_date` | `end_ts` |
| `play_store` | `ps_avail` |
| `app_store` | `as_avail` |
| `one_store` | `os_avail` |
| `pc` | `pc_avail` |
| `exposure_type` | `expose_kind` |
| `exposure_option` | `expose_opt` |
| `notices` | `bulletins` |
| `achieve_type` | `trophy_kind` |
| `quest_id` | `goal_id` |
| `reward_coin` | `bounty_token` |
| `reward_chip` | `bounty_stack` |
| `refresh_time_coin_free_charge` | `token_refuel_next` |
| `refresh_time_chip_free_charge` | `stack_refuel_next` |
| `charged_amount` | `refuel_amount` |
| `withdraw_coin` | `drain_token` (rakeback) / `take_tokens` (safe) |
| `current_rakeback` | `cashback_balance` |
| `current_coin` | `cur_token` |
| `shop_product_type` | `market_item_kind` |
| `product_id` | `item_no` |
| `developer_pay_load` | `dev_payload` |
| `receipt` | `recv_proof` |
| `signature` | `sig` |
| `is_sandbox` | `sandbox_flag` |
| `recv_mail` | `inbox_got` |
| `update_player` | `refresh_member` |
| `update_avatars` | `refresh_skins` |
| `billing_error` | `bill_fail` |
| `transaction_i_d` | `txn_id` |
| `change_lost_limit` | `loss_cap_amt` |
| `change_set_time_limit` | `block_span_swap` |
| `deposit_chip` | `stash_stacks` |
| `deposit_coin` | `stash_tokens` |
| `chip` | `stacks` |
| `safe_chip` | `vault_stacks` |
| `safe_coin` | `vault_tokens` |
| `withdraw_chip` | `take_stacks` |
| `start` (ad watch) | `start_flag` |
| `limit_game_type` | `cap_match_kind` |
| `limit_money_type` | `cap_fund_kind` |
| `mail_box_check` | `inbox_check` |
| `buy_class` | `tier_buy_flag` |
| `cut_card_number` | `cut_tile_no` |
| `players0_cards`~`players4_cards` | `m0_cards`~`m4_cards` |
| `clear_cheat` | `clear_debug` |
| `auth_code` | `auth_token` |
| `playerIdx_1`~`playerIdx_9` | `memberIdx_1`~`memberIdx_9` |
| `player_coins` | `member_tokens_map` |
| `player_chips` (map) | `member_stacks_map` |
| `playerIdx` (QA) | `memberIdx` |
| `user_infos` | `tester_infos` |
| `friend_status` | `mate_state` |
| `playing_channel_id` | `active_ch_token` |
| `delete_player_idx` | `drop_member_idx` |
| `current_delete_friend_count` | `mate_drop_cnt` |
| `lobby_players` | `atrium_members` |
| `friend_players` | `mate_members` |
| `searched_players` | `looked_up_members` |
| `version` | `ver` |
| `market_list` | `store_list` |
| `web_url` | `web_link` |
| `lobby_list` | `atrium_list` |
| `slot_list` | `reel_list` |
| `playstore_*` | `ps_*` |
| `appstore_*` | `as_*` |
| `onestore_*` | `os_*` |
| `pc_*` (version) | `pc_*` |
| `review_lobby_list` | `review_atrium_list` |
| `review_slot_list` | `review_reel_list` |
| `error_code` | `err_token` |
| `error_message` | `err_notice` |
| `index` | `idx` |
| `event_name` | `drop_name` |
| `start_time` | `start_ts` |
| `end_time` | `end_ts` |
| `log_code` | `trace_code` |
| `reward` | `bounty` |
| `reqtype` | `req_kind` |
| `event_info_list` | `drop_info_list` |
| `received_event_list` | `got_drop_list` |
| `received_event_now` | `got_drop_now` |
| `Error_Log` | `fail_trace` |
| `ranking_list` | `leaderboard_list` |
| `IsIngame` | `in_match_flag` |
| `log_list` | `trace_list` |
| `pw` | `key` |
| `event_code` | `drop_code` |
| `ballcount` | `ball_left` |
| `betmoney` | `wager_amount` |
| `goal_number` | `goal_no` |
| `lucky_number` | `lucky_no` |
| `remain_lucky_turn` | `lucky_turns_left` |
| `goal_list` | `goal_log` |
| `bingo` (map field) | `bingo_grid` |
| `reset_game` | `game_reset` |
| `pinball_id` | `marble_id` |
| `result` | `outcome` |
| `get_money` | `payout` |
| `bingo_count` | `bingo_hits` |
| `all_light` | `full_light` |
| `totalbet` | `total_wager` |
| `is_multiball` | `multiball_flag` |
| `pinballs` | `marbles` |
| `buy_after_money` | `fund_after_buy` |
| `pinballs_response` | `marbles_res` |
| `pinball_type` | `marble_kind` |
| `last_pinball_id` | `last_marble_id` |
| `have_replay` | `replay_avail` |
| `bingo_pay` | `bingo_payout` |
| `hole_num` | `hole_nums` |
| `luck_num` | `lucky_no` |
| `bingo_boad_sort` | `bingo_board_sort` |
| `goal_num` | `goal_no` |
| `roomnumber` | `chamber_no` |
| `actionnum` | `action_no` |
| `spinidx` | `rotation_idx` |
| `has_not_acked_pinball` | `pending_marbles` |
| `isReservationPlayer` | `hold_member_flag` |
| `is_watcher_reservation` | `view_hold_ok` |

---

## 가짜(decoy) 필드 추가 패턴

각 message에 최소 1개의 의미 없는 가짜 필드를 추가했습니다. 실제 사용하지 마세요.

| 가짜 필드 패턴 | 의도된 인상 |
|----------------|-------------|
| `int32 _crc = N;` | 무결성 체크값처럼 보임 |
| `int32 _v = N;` | 스키마 버전처럼 보임 |
| `int32 _shard = N;` | 샤드 ID처럼 보임 |
| `bytes _hash = N;` | 해시 값처럼 보임 |

> **모두 _ 접두사를 사용**하므로 매핑 파일을 참조하면 어느 게 가짜인지 쉽게 식별 가능.
> 정식 필드는 _ 접두사를 사용하지 않으므로 충돌 없음.

---

## 메시지 전체 매핑표 (원본 → 변환)

| 원본 이름 | 변환된 이름 |
|-----------|-------------|
| ProtoBase | PktBase |
| SysErrorRes | SysFaultRS |
| CreateAccountReq | BuildProfileRQ |
| CreateAccountRes | BuildProfileRS |
| CreatePlatformReq | BuildOutletRQ |
| CreatePlatformRes | BuildOutletRS |
| GetCreatedPlatformsByCidReq | FetchBuiltOutletsByCidRQ |
| GetCreatedPlatformsByCidRes | FetchBuiltOutletsByCidRS |
| CreateMADEPlatformReq | BuildNativeOutletRQ |
| CreateMADEPlatformRes | BuildNativeOutletRS |
| ResetSubPasswordReq | ReissueAuxKeyRQ |
| ResetSubPasswordRes | ReissueAuxKeyRS |
| UpdateTermsAgreeReq | RefreshTermsOptinRQ |
| UpdateTermsAgreeRes | RefreshTermsOptinRS |
| UpdateCiExpiryTimeReq | RefreshCiExpireRQ |
| UpdateCiExpiryTimeRes | RefreshCiExpireRS |
| WithdrawalGameReq | QuitMatchRQ |
| WithdrawalGameRes | QuitMatchRS |
| CancelWithdrawalGameReq | RevokeQuitMatchRQ |
| CancelWithdrawalGameRes | RevokeQuitMatchRS |
| LoginReq | SigninRQ |
| ReLoginRes | ResumeSigninRS |
| Records | Ledger |
| LoginRes | SigninRS |
| SessionLogoutReq | TunnelSignoffRQ |
| SessionLogoutRes | TunnelSignoffRS |
| DuplicatedSessionReq | DupTunnelRQ |
| DuplicatedSessionRes | DupTunnelRS |
| VersionNotiReq | BuildBulletinRQ |
| VersionNotiRes | BuildBulletinRS |
| TransferServerReq | MigrateNodeRQ |
| TransferServerRes | MigrateNodeRS |
| MoveLobbyRes | ShiftAtriumRS |
| SystemMessage | ServiceNotice |
| MADEPassChangeReq | NativeKeySwapRQ |
| MADEPassChangeRes | NativeKeySwapRS |
| MADECheckIdDuplicateReq | NativeIdDupVerifyRQ |
| MADECheckIdDuplicateRes | NativeIdDupVerifyRS |
| NickChangeReq | AliasSwapRQ |
| NickChangeRes | AliasSwapRS |
| EnterGameReq | OpenMatchRQ |
| GetPlayerInfoReq | FetchMemberDetailRQ |
| GetPlayerInfoRes | FetchMemberDetailRS |
| PingReq | HeartbeatRQ |
| PingRes | HeartbeatRS |
| RoomJoinReq | ChamberEnterRQ |
| RoomJoinRes | ChamberEnterRS |
| MoveRoomReq | ShiftChamberRQ |
| GameParticipateReq | MatchEngageRQ |
| CancelGameParticipateReq | RevokeMatchEngageRQ |
| CancelGameParticipateRes | RevokeMatchEngageRS |
| TrasferToWatcherReq | ToObserverShiftRQ |
| TrasferToWatcherRes | ToObserverShiftRS |
| RoomJoinAsWatcherReq | ChamberEnterAsObserverRQ |
| CancelTrasferToWatcherReq | RevokeToObserverShiftRQ |
| CancelTrasferToWatcherRes | RevokeToObserverShiftRS |
| RemovePlayerOnSlotRes | DropMemberOnSeatRS |
| KickOutPlayerReq | ExpelMemberRQ |
| KickOutPlayerRes | ExpelMemberRS |
| CancelKickOutPlayerReq | RevokeExpelMemberRQ |
| CancelKickOutPlayerRes | RevokeExpelMemberRS |
| NotifyWatcherCountRes | InformObserverCntRS |
| RoomOutReq | ChamberLeaveRQ |
| RoomOutRes | ChamberLeaveRS |
| RoomOutReserveReq | ChamberLeaveHoldRQ |
| RoomOutReserveRes | ChamberLeaveHoldRS |
| RoomListReq | ChamberIndexRQ |
| RoomListRes | ChamberIndexRS |
| PlayTurnRes | PhaseTurnRS |
| PlayBetReq | MatchWagerRQ |
| PlayBetRes | MatchWagerRS |
| SideBetRes | FlankWagerRS |
| BaseDistributionRes | SeedDealoutRS |
| PlayStatusChangeRes | MatchStateSwapRS |
| PlayBossChangeRes | MatchLeadSwapRS |
| PlayStartReq | MatchKickoffRQ |
| PlayStartRes | MatchKickoffRS |
| PlayCardChangeReq | MatchHandSwapRQ |
| PlayCardChangeRes | MatchHandSwapRS |
| CommunityCardRes | SharedTileRS |
| BlackjackWinStatus (enum) | BJOutcomeKind |
| BlackjackPPStatus (enum) | BJSideOutcomeKind |
| BlackjackPlayResult | BJMatchOutcome |
| BlackjackPlayPPResult | BJSideOutcome |
| PlayerResult | MemberOutcome |
| PlayResultRes | MatchOutcomeRS |
| PlayPPResultRes | MatchSideOutcomeRS |
| PlayRemainTimeRes | PhaseRemainRS |
| RoomHistoryUpdateRes | ChamberLogRefreshRS |
| RoomRightToVoteRes | ChamberPollEligibleRS |
| RoomTryVoteReq | ChamberPollAttemptRQ |
| RoomTryVoteRes | ChamberPollAttemptRS |
| RoomVoteAnswerReq | ChamberPollReplyRQ |
| RoomVoteAnswerRes | ChamberPollReplyRS |
| RoomVoteResultRes | ChamberPollOutcomeRS |
| RoomCreateReq | ChamberBuildRQ |
| RoomCreateRes | ChamberBuildRS |
| FriendRoomCreateReq | MateChamberBuildRQ |
| FriendRoomCreateRes | MateChamberBuildRS |
| SendEmoticonReq | PushStickerRQ |
| SendEmoticonRes | PushStickerRS |
| ShowDownRes | RevealRS |
| BaccaratCardChangeRes | BCTileSwapRS |
| BaccaratPairNaturalRes | BCCoupleNtlRS |
| CancelBetReq | RevokeWagerRQ |
| CancelBetRes | RevokeWagerRS |
| BaccaratPassBetReq | BCSkipWagerRQ |
| BaccaratPassBetRes | BCSkipWagerRS |
| BaccaratDoubleBetReq | BCTwinWagerRQ |
| BaccaratDoubleBetRes | BCTwinWagerRS |
| BaccaratCallLastBetReq | BCHailFinalWagerRQ |
| BaccaratCallLastBetRes | BCHailFinalWagerRS |
| PlayMasterChangeRes | MatchCaptainSwapRS |
| LeftCardNotiRes | TileRemainBulletinRS |
| MoveSlotReq | SeatSlideRQ |
| MoveSlotRes | SeatSlideRS |
| BlackjackCardChangeRes | BJTileSwapRS |
| BlackjackCardUpdateRes | BJTileRefreshRS |
| BlackjackDealingActionReq | BJServeMoveRQ |
| BlackjackDealingActionRes | BJServeMoveRS |
| BlackjackPopupRes | BJBannerRS |
| BlackjackCreateSubReq | BJBuildAuxRQ |
| BlackjackCreateSubRes | BJBuildAuxRS |
| ResultCompleteReq | OutcomeCompleteRQ |
| CheatMoneyReq | DebugFundsRQ |
| CheatMoneyRes | DebugFundsRS |
| CheatCardChangeReq | DebugHandSwapRQ |
| CheatCardChangeRes | DebugHandSwapRS |
| CheatSlotReq | DebugReelRQ |
| CheatSlotRes | DebugReelRS |
| CheatSlotRankingReq | DebugReelBoardRQ |
| Spin | Rotation |
| SpinReq | RotationRQ |
| SpinRes | RotationRS |
| RouletteSpinReq | DiscRotationRQ |
| RouletteSpinRes | DiscRotationRS |
| SelectSpinReq | ChooseRotationRQ |
| SelectSpinRes | ChooseRotationRS |
| PickGameChoiceReq | SnagMatchSelectRQ |
| PickGameChoiceRes | SnagMatchSelectRS |
| EnterSlotGameReq | OpenReelMatchRQ |
| EnterSlotGameRes | OpenReelMatchRS |
| SpinAckReq | RotationEchoRQ |
| SpinAckRes | RotationEchoRS |
| LeaveSlotGameReq | ExitReelMatchRQ |
| LeaveSlotGameRes | ExitReelMatchRS |
| MakeAckRemainSpinsReq | BuildEchoLeftRotationsRQ |
| MakeAckRemainSpinsRes | BuildEchoLeftRotationsRS |
| GetVirtualCoinReq | FetchPhantomTokenRQ |
| GetVirtualCoinRes | FetchPhantomTokenRS |
| RePlayFreeSpinsReq | ReplayBonusRotationsRQ |
| RePlayFreeSpinsRes | ReplayBonusRotationsRS |
| SlotLoginReq | ReelSigninRQ |
| SlotLoginRes | ReelSigninRS |
| PlayerSetAvatarReq | MemberApplySkinRQ |
| PlayerSetAvatarRes | MemberApplySkinRS |
| PlayerSetSubPasswdReq | MemberApplyAuxKeyRQ |
| PlayerSetSubPasswdRes | MemberApplyAuxKeyRS |
| PlayerChangeSubPasswdReq | MemberSwapAuxKeyRQ |
| PlayerChangeSubPasswdRes | MemberSwapAuxKeyRS |
| PlayerRemoveSubPasswdReq | MemberDropAuxKeyRQ |
| PlayerRemoveSubPasswdRes | MemberDropAuxKeyRS |
| UpdateLobbyReq | RefreshAtriumRQ |
| UpdateLobbyRes | RefreshAtriumRS |
| UpdateSlotReq | RefreshReelRQ |
| UpdateSlotRes | RefreshReelRS |
| MailInfo | InboxDetail |
| MailBoxReq | InboxIndexRQ |
| MailBoxRes | InboxIndexRS |
| MailOpenReq | InboxReadRQ |
| MailOpenRes | InboxReadRS |
| NoticeMessageInfo | BulletinDetail |
| NoticeMessageReq | BulletinIndexRQ |
| NoticeMessageRes | BulletinIndexRS |
| GetQuestsReq | FetchGoalsRQ |
| GetQuestsRes | FetchGoalsRS |
| GetQuestRewardReq | FetchGoalBountyRQ |
| GetQuestRewardRes | FetchGoalBountyRS |
| GetFreeChargeReq | FetchBonusRefuelRQ |
| GetFreeChargeRes | FetchBonusRefuelRS |
| WithdrawRakebackReq | DrainCashbackRQ |
| WithdrawRakebackRes | DrainCashbackRS |
| BuyShopReq | MarketPurchaseRQ |
| BuyShopRes | MarketPurchaseRS |
| ChangeLostLimitReq | SwapLossCapRQ |
| ChangeLostLimitRes | SwapLossCapRS |
| CheckLostLimitTimeReq | VerifyLossCapSpanRQ |
| CheckLostLimitTimeRes | VerifyLossCapSpanRS |
| SafeMoneyDepositReq | VaultStashRQ |
| SafeMoneyDepositRes | VaultStashRS |
| SafeMoneyWithdrawReq | VaultTakeRQ |
| SafeMoneyWithdrawRes | VaultTakeRS |
| AdWatchReq | PromoViewRQ |
| AdWatchRes | PromoViewRS |
| MoneyLimitPopopOnResultRes | FundCapBannerRS |
| LostLimitPopopOnResultRes | LossCapBannerRS |
| BlackjackCheatReq | BJDebugRQ |
| BlackjackCheatRes | BJDebugRS |
| BaccaratCheatReq | BCDebugRQ |
| BaccaratCheatRes | BCDebugRS |
| QALoginReq | TesterSigninRQ |
| QALoginRes | TesterSigninRS |
| CardDeckCheatReq | StackDebugRQ |
| CardDeckCheatRes | StackDebugRS |
| QAUserInfo | TesterMemberDetail |
| UidSearchByNicknameReq | MidLookupByAliasRQ |
| UidSearchByNicknameRes | MidLookupByAliasRS |
| InitLostLimitReq | ResetLossCapRQ |
| InitLostLimitRes | ResetLossCapRS |
| InitBuyLimitReq | ResetSpendCapRQ |
| InitBuyLimitRes | ResetSpendCapRS |
| FriendInfo | MateDetail |
| AddFriendReq | InsertMateRQ |
| AddFriendRes | InsertMateRS |
| DeleteFriendReq | DropMateRQ |
| DeleteFriendRes | DropMateRS |
| LobbyPlayerListReq | AtriumMemberIndexRQ |
| LobbyPlayerListRes | AtriumMemberIndexRS |
| FriendListReq | MateIndexRQ |
| FriendListRes | MateIndexRS |
| SearchPlayerReq | LookupMemberRQ |
| SearchPlayerRes | LookupMemberRS |
| FrontReq | PortalRQ |
| FrontRes | PortalRS |
| EventInfo | DropDetail |
| ReceivedEvent | GotDrop |
| LoginRewardReq | SigninBountyRQ |
| LoginRewardRes | SigninBountyRS |
| SyncUserInfoReq | AlignMemberDetailRQ |
| SyncUserInfoRes | AlignMemberDetailRS |
| ForcedMessage | MandatoryNotice |
| ErrorLogReq | FailTraceRQ |
| SlotRankingEventReq | ReelLeaderboardDropRQ |
| SlotRankingEventRes | ReelLeaderboardDropRS |
| SlotEpicWinElectronicDisplay | ReelMegaWinBoard |
| LoadUserListRes | PullMemberIndexRS |
| SaveUserListReq | StoreMemberIndexRQ |
| SaveUserListRes | StoreMemberIndexRS |
| ChangeMailStateReq | SwapInboxStateRQ |
| SlotRewardEvent | ReelBountyDrop |
| Pinball | Marble |
| PinballResult | MarbleOutcome |
| Bingo (enum) | GridCell |
| PinballReq | MarbleRQ |
| PinballRes | MarbleRS |
| RePlayPinballReq | ReplayMarbleRQ |
| RePlayPinballRes | ReplayMarbleRS |
| EnterPinballReq | OpenMarbleRQ |
| EnterPinballRes | OpenMarbleRS |
| PinballAckReq | MarbleEchoRQ |
| LeavePinballReq | ExitMarbleRQ |
| LeavePinballRes | ExitMarbleRS |
| PinballQARes | MarbleTesterRS |
| PinballQAReq | MarbleTesterRQ |
| RouletteQAReq | DiscTesterRQ |
| RouletteQARes | DiscTesterRS |
| ChangeDailyExpiredReq | SwapDailyStaleRQ |
| ChangeDailyExpiredRes | SwapDailyStaleRS |
| RecordedPinball | LoggedMarble |
| PinballRecords | MarbleLog |
| RouletteNumRes | DiscNumRS |
| UpdatePushRes | RefreshAlertRS |
| UpdatePushReq | RefreshAlertRQ |

---

## Enum 값 매핑

### BlackjackWinStatus → BJOutcomeKind
| 원본 | 변환 |
|------|------|
| BlackjackWinStatus_None | BJOutcomeKind_None |
| Win | Win (유지) |
| Lose | Lose (유지) |
| Draw | Draw (유지) |
| EvenMoney | EvenMoney (유지) |

### BlackjackPPStatus → BJSideOutcomeKind
| 원본 | 변환 |
|------|------|
| BlackjackPPStatus_None | BJSideOutcomeKind_None |
| Mix | Mix (유지) |
| Color | Color (유지) |
| Perfect | Perfect (유지) |

### Bingo → GridCell
| 원본 | 변환 |
|------|------|
| B_None | GC_None |
| B_1_1 | GC_1_1 |
| B_1_2 | GC_1_2 |
| ... (B_R_C 모두 GC_R_C 로 치환) | ... |
| B_4_4 | GC_4_4 |

> Enum 정수값은 변경하지 않았습니다 (0, 11, 12, …, 44 그대로 유지).

---

## 필드 번호 변경 정책

각 message 단위로 다음 규칙을 사용했습니다:

- 원본 `= 1, 2, 3, 4, …, 10` → 새 번호는 같은 message 내에서 **재배치**.
- 일관성 있는 패턴 대신 message마다 약간씩 다른 순서로 섞어 추측을 어렵게 함.
- 가짜 필드는 보통 마지막 사용 번호보다 큰 값에 배치 (예: `_crc = 9, 15, 20`).
- 일부 message는 원래 값과 동일한 위치를 유지하기도 함 (모두 다르게 만들면 패턴 자체가 추측 단서가 되므로 자연스러운 분포 유지).

> **wire 호환성 단절**이 의도이므로, 동일 필드라도 message에 따라 번호가 다를 수 있습니다.

---

## 복원 방법

1. 이 메모의 메시지/필드 매핑표를 사용해 원래 이름으로 되돌릴 수 있습니다.
2. `_` 접두사가 붙은 필드는 모두 제거하면 가짜 필드가 사라집니다.
3. 필드 번호는 원본 파일의 git history (또는 백업)에서 복구해야 합니다 — 이 메모에는 메시지별 번호 매핑을 따로 기록하지 않았습니다.

---

## 위험 신호

- 위 변환을 적용한 후 **반드시 protoc 컴파일 검증**을 수행하세요.
- 클라이언트와 서버 양쪽 코드에서 변경된 message/필드 이름을 모두 갱신해야 합니다.
- `Common.proto` 는 변경하지 않았으므로 import 문은 그대로 둡니다.
- Korean 주석 (한글) 은 모두 원본 그대로 유지했습니다.
