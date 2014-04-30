/*
 * Generated on Wed Apr 30 20:24:16 2014 by enum-msg.pl -- DO NOT EDIT
 *
 * Command: ../../../scripts/enum-msg.pl stats.lst
 */

#ifndef _if_gen_gnr_stats_h_
#define _if_gen_gnr_stats_h_

/*
 * Enum count: 298
 */
typedef enum {
	GNR_ROUTING_ERRORS = 0,
	GNR_ROUTING_TABLE_CHUNKS,
	GNR_ROUTING_TABLE_CAPACITY,
	GNR_ROUTING_TABLE_COUNT,
	GNR_ROUTING_TRANSIENT_AVOIDED,
	GNR_DUPS_WITH_HIGHER_TTL,
	GNR_SPAM_SHA1_HITS,
	GNR_SPAM_NAME_HITS,
	GNR_SPAM_FAKE_HITS,
	GNR_SPAM_DUP_HITS,
	GNR_SPAM_CAUGHT_HOSTILE_IP,
	GNR_SPAM_CAUGHT_HOSTILE_HELD,
	GNR_SPAM_IP_HELD,
	GNR_LOCAL_SEARCHES,
	GNR_LOCAL_HITS,
	GNR_LOCAL_PARTIAL_HITS,
	GNR_LOCAL_WHATS_NEW_HITS,
	GNR_LOCAL_QUERY_HITS,
	GNR_LOCAL_G2_SEARCHES,
	GNR_LOCAL_G2_HITS,
	GNR_LOCAL_G2_PARTIAL_HITS,
	GNR_OOB_PROXIED_QUERY_HITS,
	GNR_OOB_QUERIES,
	GNR_OOB_QUERIES_STRIPPED,
	GNR_OOB_QUERIES_IGNORED,
	GNR_QUERY_OOB_PROXIED_DUPS,
	GNR_OOB_HITS_FOR_PROXIED_QUERIES,
	GNR_OOB_HITS_WITH_ALIEN_IP,
	GNR_OOB_HITS_IGNORED_ON_SPAMMER_HIT,
	GNR_OOB_HITS_IGNORED_ON_UNSECURE_HIT,
	GNR_UNCLAIMED_OOB_HITS,
	GNR_PARTIALLY_CLAIMED_OOB_HITS,
	GNR_SPURIOUS_OOB_HIT_CLAIM,
	GNR_UNREQUESTED_OOB_HITS,
	GNR_QUERY_HIT_FOR_UNTRACKED_QUERY,
	GNR_QUERY_TRACKED_MUIDS,
	GNR_QUERY_COMPACT_COUNT,
	GNR_QUERY_COMPACT_SIZE,
	GNR_QUERY_UTF8,
	GNR_QUERY_SHA1,
	GNR_QUERY_WHATS_NEW,
	GNR_QUERY_G2_UTF8,
	GNR_QUERY_G2_SHA1,
	GNR_QUERY_GUESS,
	GNR_QUERY_GUESS_02,
	GNR_GUESS_LINK_CACHE,
	GNR_GUESS_CACHED_QUERY_KEYS_HELD,
	GNR_GUESS_CACHED_02_HOSTS_HELD,
	GNR_GUESS_CACHED_G2_HOSTS_HELD,
	GNR_GUESS_LOCAL_QUERIES,
	GNR_GUESS_LOCAL_RUNNING,
	GNR_GUESS_LOCAL_QUERY_HITS,
	GNR_GUESS_ULTRA_QUERIED,
	GNR_GUESS_ULTRA_ACKNOWLEDGED,
	GNR_GUESS_G2_QUERIED,
	GNR_GUESS_G2_ACKNOWLEDGED,
	GNR_BROADCASTED_PUSHES,
	GNR_PUSH_PROXY_UDP_RELAYED,
	GNR_PUSH_PROXY_TCP_RELAYED,
	GNR_PUSH_PROXY_TCP_FW2FW,
	GNR_PUSH_PROXY_BROADCASTED,
	GNR_PUSH_PROXY_ROUTE_NOT_PROXIED,
	GNR_PUSH_PROXY_FAILED,
	GNR_PUSH_RELAYED_VIA_LOCAL_ROUTE,
	GNR_PUSH_RELAYED_VIA_TABLE_ROUTE,
	GNR_LOCAL_DYN_QUERIES,
	GNR_LEAF_DYN_QUERIES,
	GNR_OOB_PROXIED_QUERIES,
	GNR_DYN_QUERIES_COMPLETED_FULL,
	GNR_DYN_QUERIES_COMPLETED_PARTIAL,
	GNR_DYN_QUERIES_COMPLETED_ZERO,
	GNR_DYN_QUERIES_LINGER_EXTRA,
	GNR_DYN_QUERIES_LINGER_RESULTS,
	GNR_DYN_QUERIES_LINGER_COMPLETED,
	GNR_GTKG_TOTAL_QUERIES,
	GNR_GTKG_REQUERIES,
	GNR_QUERIES_WITH_GGEP_H,
	GNR_QUERIES_WITH_SR_UDP,
	GNR_GIV_CALLBACKS,
	GNR_GIV_DISCARDED,
	GNR_QUEUE_CALLBACKS,
	GNR_QUEUE_DISCARDED,
	GNR_BANNED_FDS_TOTAL,
	GNR_UDP_READ_AHEAD_COUNT_SUM,
	GNR_UDP_READ_AHEAD_BYTES_SUM,
	GNR_UDP_READ_AHEAD_OLD_SUM,
	GNR_UDP_READ_AHEAD_COUNT_MAX,
	GNR_UDP_READ_AHEAD_BYTES_MAX,
	GNR_UDP_READ_AHEAD_DELAY_MAX,
	GNR_UDP_FW2FW_PUSHES,
	GNR_UDP_FW2FW_PUSHES_TO_SELF,
	GNR_UDP_FW2FW_PUSHES_PATCHED,
	GNR_UDP_UHC_PINGS,
	GNR_UDP_UHC_PONGS,
	GNR_UDP_BOGUS_SOURCE_IP,
	GNR_UDP_RX_TRUNCATED,
	GNR_UDP_ALIEN_MESSAGE,
	GNR_UDP_UNPROCESSED_MESSAGE,
	GNR_UDP_TX_COMPRESSED,
	GNR_UDP_RX_COMPRESSED,
	GNR_UDP_LARGER_HENCE_NOT_COMPRESSED,
	GNR_UDP_AMBIGUOUS,
	GNR_UDP_AMBIGUOUS_DEEPER_INSPECTION,
	GNR_UDP_AMBIGUOUS_AS_SEMI_RELIABLE,
	GNR_UDP_SR_TX_MESSAGES_GIVEN,
	GNR_UDP_SR_TX_MESSAGES_DEFLATED,
	GNR_UDP_SR_TX_MESSAGES_UNSENT,
	GNR_UDP_SR_TX_MESSAGES_BANNED,
	GNR_UDP_SR_TX_MESSAGES_GOOD,
	GNR_UDP_SR_TX_MESSAGES_CLOGGING,
	GNR_UDP_SR_TX_RELIABLE_MESSAGES_GIVEN,
	GNR_UDP_SR_TX_RELIABLE_MESSAGES_SENT,
	GNR_UDP_SR_TX_RELIABLE_MESSAGES_UNSENT,
	GNR_UDP_SR_TX_FRAGMENTS_SENT,
	GNR_UDP_SR_TX_FRAGMENTS_RESENT,
	GNR_UDP_SR_TX_FRAGMENTS_SENDING_AVOIDED,
	GNR_UDP_SR_TX_FRAGMENTS_OVERSENT,
	GNR_UDP_SR_TX_TOTAL_ACKS_RECEIVED,
	GNR_UDP_SR_TX_CUMULATIVE_ACKS_RECEIVED,
	GNR_UDP_SR_TX_EXTENDED_ACKS_RECEIVED,
	GNR_UDP_SR_TX_SPURIOUS_ACKS_RECEIVED,
	GNR_UDP_SR_TX_INVALID_ACKS_RECEIVED,
	GNR_UDP_SR_TX_ACKS_REQUEUED,
	GNR_UDP_SR_TX_PLAIN_ACKS_DROPPED,
	GNR_UDP_SR_TX_ENHANCED_ACKS_DROPPED,
	GNR_UDP_SR_TX_PLAIN_ACKS_SENT,
	GNR_UDP_SR_TX_CUMULATIVE_ACKS_SENT,
	GNR_UDP_SR_TX_EXTENDED_ACKS_SENT,
	GNR_UDP_SR_TX_EARS_SENT,
	GNR_UDP_SR_TX_EARS_OVERSENT,
	GNR_UDP_SR_TX_EAR_NACKS_RECEIVED,
	GNR_UDP_SR_TX_EAR_FOLLOWED_BY_ACKS,
	GNR_UDP_SR_RX_FRAGMENTS_RECEIVED,
	GNR_UDP_SR_RX_FRAGMENTS_DUPLICATE,
	GNR_UDP_SR_RX_FRAGMENTS_UNRELIABLE,
	GNR_UDP_SR_RX_FRAGMENTS_DROPPED,
	GNR_UDP_SR_RX_FRAGMENTS_LINGERING,
	GNR_UDP_SR_RX_MESSAGES_EXPIRED,
	GNR_UDP_SR_RX_MESSAGES_RECEIVED,
	GNR_UDP_SR_RX_MESSAGES_INFLATED,
	GNR_UDP_SR_RX_MESSAGES_INFLATION_ERROR,
	GNR_UDP_SR_RX_MESSAGES_UNRELIABLE,
	GNR_UDP_SR_RX_MESSAGES_EMPTY,
	GNR_UDP_SR_RX_TOTAL_ACKS_SENT,
	GNR_UDP_SR_RX_CUMULATIVE_ACKS_SENT,
	GNR_UDP_SR_RX_EXTENDED_ACKS_SENT,
	GNR_UDP_SR_RX_AVOIDED_ACKS,
	GNR_UDP_SR_RX_EARS_RECEIVED,
	GNR_UDP_SR_RX_EARS_FOR_UNKNOWN_MESSAGE,
	GNR_UDP_SR_RX_EARS_FOR_LINGERING_MESSAGE,
	GNR_UDP_SR_RX_FROM_HOSTILE_IP,
	GNR_UDP_G2_HITS_REROUTED_TO_HUB,
	GNR_UDP_G2_HITS_UNDELIVERED,
	GNR_CONSOLIDATED_SERVERS,
	GNR_DUP_DOWNLOADS_IN_CONSOLIDATION,
	GNR_DISCOVERED_SERVER_GUID,
	GNR_CHANGED_SERVER_GUID,
	GNR_GUID_COLLISIONS,
	GNR_OWN_GUID_COLLISIONS,
	GNR_BANNED_GUID_HELD,
	GNR_RECEIVED_KNOWN_FW_NODE_INFO,
	GNR_REVITALIZED_PUSH_ROUTES,
	GNR_COLLECTED_PUSH_PROXIES,
	GNR_ATTEMPTED_RESOURCE_SWITCHING,
	GNR_ATTEMPTED_RESOURCE_SWITCHING_AFTER_ERROR,
	GNR_SUCCESSFUL_RESOURCE_SWITCHING,
	GNR_SUCCESSFUL_PLAIN_RESOURCE_SWITCHING,
	GNR_SUCCESSFUL_RESOURCE_SWITCHING_AFTER_ERROR,
	GNR_QUEUED_AFTER_SWITCHING,
	GNR_SUNK_DATA,
	GNR_IGNORED_DATA,
	GNR_IGNORING_AFTER_MISMATCH,
	GNR_IGNORING_TO_PRESERVE_CONNECTION,
	GNR_IGNORING_DURING_AGGRESSIVE_SWARMING,
	GNR_IGNORING_REFUSED,
	GNR_CLIENT_RESOURCE_SWITCHING,
	GNR_CLIENT_PLAIN_RESOURCE_SWITCHING,
	GNR_CLIENT_FOLLOWUP_AFTER_ERROR,
	GNR_PARQ_SLOT_RESOURCE_SWITCHING,
	GNR_PARQ_RETRY_AFTER_VIOLATION,
	GNR_PARQ_RETRY_AFTER_KICK_OUT,
	GNR_PARQ_SLOT_LIMIT_OVERRIDES,
	GNR_PARQ_QUICK_SLOTS_GRANTED,
	GNR_PARQ_QUEUE_SENDING_ATTEMPTS,
	GNR_PARQ_QUEUE_SENT,
	GNR_PARQ_QUEUE_FOLLOW_UPS,
	GNR_SHA1_VERIFICATIONS,
	GNR_TTH_VERIFICATIONS,
	GNR_QHIT_SEEDING_OF_ORPHAN,
	GNR_UPLOAD_SEEDING_OF_ORPHAN,
	GNR_RUDP_TX_BYTES,
	GNR_RUDP_RX_BYTES,
	GNR_DHT_ESTIMATED_SIZE,
	GNR_DHT_ESTIMATED_SIZE_STDERR,
	GNR_DHT_KBALL_THEORETICAL,
	GNR_DHT_KBALL_FURTHEST,
	GNR_DHT_KBALL_CLOSEST,
	GNR_DHT_ROUTING_BUCKETS,
	GNR_DHT_ROUTING_LEAVES,
	GNR_DHT_ROUTING_MAX_DEPTH,
	GNR_DHT_ROUTING_GOOD_NODES,
	GNR_DHT_ROUTING_STALE_NODES,
	GNR_DHT_ROUTING_PENDING_NODES,
	GNR_DHT_ROUTING_EVICTED_NODES,
	GNR_DHT_ROUTING_EVICTED_FIREWALLED_NODES,
	GNR_DHT_ROUTING_EVICTED_QUOTA_NODES,
	GNR_DHT_ROUTING_PROMOTED_PENDING_NODES,
	GNR_DHT_ROUTING_PINGED_PROMOTED_NODES,
	GNR_DHT_ROUTING_REJECTED_NODE_BUCKET_QUOTA,
	GNR_DHT_ROUTING_REJECTED_NODE_GLOBAL_QUOTA,
	GNR_DHT_COMPLETED_BUCKET_REFRESH,
	GNR_DHT_FORCED_BUCKET_REFRESH,
	GNR_DHT_FORCED_BUCKET_MERGE,
	GNR_DHT_DENIED_UNSPLITABLE_BUCKET_REFRESH,
	GNR_DHT_BUCKET_ALIVE_CHECK,
	GNR_DHT_ALIVE_PINGS_TO_GOOD_NODES,
	GNR_DHT_ALIVE_PINGS_TO_STALE_NODES,
	GNR_DHT_ALIVE_PINGS_TO_SHUTDOWNING_NODES,
	GNR_DHT_ALIVE_PINGS_AVOIDED,
	GNR_DHT_ALIVE_PINGS_SKIPPED,
	GNR_DHT_REVITALIZED_STALE_NODES,
	GNR_DHT_REJECTED_VALUE_ON_QUOTA,
	GNR_DHT_REJECTED_VALUE_ON_CREATOR,
	GNR_DHT_LOOKUP_REJECTED_NODE_ON_NET_QUOTA,
	GNR_DHT_LOOKUP_REJECTED_NODE_ON_PROXIMITY,
	GNR_DHT_LOOKUP_REJECTED_NODE_ON_DIVERGENCE,
	GNR_DHT_LOOKUP_FIXED_NODE_CONTACT,
	GNR_DHT_KEYS_HELD,
	GNR_DHT_CACHED_KEYS_HELD,
	GNR_DHT_VALUES_HELD,
	GNR_DHT_CACHED_KUID_TARGETS_HELD,
	GNR_DHT_CACHED_ROOTS_HELD,
	GNR_DHT_CACHED_ROOTS_EXACT_HITS,
	GNR_DHT_CACHED_ROOTS_APPROXIMATE_HITS,
	GNR_DHT_CACHED_ROOTS_MISSES,
	GNR_DHT_CACHED_ROOTS_KBALL_LOOKUPS,
	GNR_DHT_CACHED_ROOTS_CONTACT_REFRESHED,
	GNR_DHT_CACHED_TOKENS_HELD,
	GNR_DHT_CACHED_TOKENS_HITS,
	GNR_DHT_STABLE_NODES_HELD,
	GNR_DHT_FETCH_LOCAL_HITS,
	GNR_DHT_FETCH_LOCAL_CACHED_HITS,
	GNR_DHT_RETURNED_EXPANDED_VALUES,
	GNR_DHT_RETURNED_SECONDARY_KEYS,
	GNR_DHT_CLAIMED_SECONDARY_KEYS,
	GNR_DHT_RETURNED_EXPANDED_CACHED_VALUES,
	GNR_DHT_RETURNED_CACHED_SECONDARY_KEYS,
	GNR_DHT_CLAIMED_CACHED_SECONDARY_KEYS,
	GNR_DHT_PUBLISHED,
	GNR_DHT_REMOVED,
	GNR_DHT_STALE_REPLICATION,
	GNR_DHT_REPLICATION,
	GNR_DHT_REPUBLISH,
	GNR_DHT_SECONDARY_KEY_FETCH,
	GNR_DHT_DUP_VALUES,
	GNR_DHT_KUID_COLLISIONS,
	GNR_DHT_OWN_KUID_COLLISIONS,
	GNR_DHT_CACHING_ATTEMPTS,
	GNR_DHT_CACHING_SUCCESSFUL,
	GNR_DHT_CACHING_PARTIALLY_SUCCESSFUL,
	GNR_DHT_KEY_OFFLOADING_CHECKS,
	GNR_DHT_KEYS_SELECTED_FOR_OFFLOADING,
	GNR_DHT_KEY_OFFLOADING_ATTEMPTS,
	GNR_DHT_KEY_OFFLOADING_SUCCESSFUL,
	GNR_DHT_KEY_OFFLOADING_PARTIALLY_SUCCESSFUL,
	GNR_DHT_VALUES_OFFLOADED,
	GNR_DHT_MSG_RECEIVED,
	GNR_DHT_MSG_MATCHING_CONTACT_ADDRESS,
	GNR_DHT_MSG_FIXED_CONTACT_ADDRESS,
	GNR_DHT_MSG_FROM_HOSTILE_ADDRESS,
	GNR_DHT_MSG_FROM_HOSTILE_CONTACT_ADDRESS,
	GNR_DHT_RPC_MSG_PREPARED,
	GNR_DHT_RPC_MSG_CANCELLED,
	GNR_DHT_RPC_TIMED_OUT,
	GNR_DHT_RPC_REPLIES_RECEIVED,
	GNR_DHT_RPC_REPLIES_FIXED_CONTACT,
	GNR_DHT_RPC_LATE_REPLIES_RECEIVED,
	GNR_DHT_RPC_KUID_REPLY_MISMATCH,
	GNR_DHT_RPC_RECENT_NODES_HELD,
	GNR_DHT_NODE_VERIFICATIONS,
	GNR_DHT_PUBLISHING_ATTEMPTS,
	GNR_DHT_PUBLISHING_SUCCESSFUL,
	GNR_DHT_PUBLISHING_PARTIALLY_SUCCESSFUL,
	GNR_DHT_PUBLISHING_SATISFACTORY,
	GNR_DHT_REPUBLISHED_LATE,
	GNR_DHT_PUBLISHING_TO_SELF,
	GNR_DHT_PUBLISHING_BG_ATTEMPTS,
	GNR_DHT_PUBLISHING_BG_IMPROVEMENTS,
	GNR_DHT_PUBLISHING_BG_SUCCESSFUL,
	GNR_DHT_SHA1_DATA_TYPE_COLLISIONS,
	GNR_DHT_PASSIVELY_PROTECTED_LOOKUP_PATH,
	GNR_DHT_ACTIVELY_PROTECTED_LOOKUP_PATH,
	GNR_DHT_ALT_LOC_LOOKUPS,
	GNR_DHT_PUSH_PROXY_LOOKUPS,
	GNR_DHT_SUCCESSFUL_ALT_LOC_LOOKUPS,
	GNR_DHT_SUCCESSFUL_PUSH_PROXY_LOOKUPS,
	GNR_DHT_SUCCESSFUL_NODE_PUSH_ENTRY_LOOKUPS,
	GNR_DHT_SEEDING_OF_ORPHAN,

	GNR_TYPE_COUNT
} gnr_stats_t;

const char *gnet_stats_general_to_string(gnr_stats_t x);

const char *gnet_stats_general_description(gnr_stats_t x);

#endif /* _if_gen_gnr_stats_h_ */

/* vi: set ts=4 sw=4 cindent: */
