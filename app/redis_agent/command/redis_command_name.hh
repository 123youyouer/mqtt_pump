//
// Created by root on 2020/5/15.
//

#ifndef PUMP_REDIS_COMMAND_NAME_HH
#define PUMP_REDIS_COMMAND_NAME_HH

#include <string>

namespace redis_agent::command{
    enum class redis_cmd_name
    {
        None = 0,
        Ping,
        PingServ,
        Echo,
        Auth,
        AuthServ,
        Select,
        SelectServ,
        Quit,

        SentinelSentinels,
        SentinelGetMaster,
        SentinelSlaves,

        Cmd,
        Info,
        Config,

        Cluster,
        ClusterNodes,
        Asking,
        Readonly,

        Watch,
        Unwatch,
        UnwatchServ,
        Multi,
        Exec,
        Discard,
        DiscardServ,

        Eval,
        Evalsha,
        Script,
        ScriptLoad,

        Del,
        Dump,
        Exists,
        Expire,
        Expireat,
        Move,
        Persist,
        Pexpire,
        Pexpireat,
        Pttl,
        Randomkey,
        Rename,
        Renamenx,
        Restore,
        Sort,
        Touch,
        Ttl,
        TypeCmd,
        Unlink,
        Scan,
        Append,
        Bitcount,
        Bitfield,
        Bitop,
        Bitpos,
        Decr,
        Decrby,
        Get,
        Getbit,
        Getrange,
        Getset,
        Incr,
        Incrby,
        Incrbyfloat,
        Mget,
        Mset,
        Msetnx,
        Psetex,
        Set,
        Setbit,
        Setex,
        Setnx,
        Setrange,
        Strlen,

        Hdel,
        Hexists,
        Hget,
        Hgetall,
        Hincrby,
        Hincrbyfloat,
        Hkeys,
        Hlen,
        Hmget,
        Hmset,
        Hscan,
        Hset,
        Hsetnx,
        Hstrlen,
        Hvals,

        Blpop,
        Brpop,
        Brpoplpush,
        Lindex,
        Linsert,
        Llen,
        Lpop,
        Lpush,
        Lpushx,
        Lrange,
        Lrem,
        Lset,
        Ltrim,
        Rpop,
        Rpoplpush,
        Rpush,
        Rpushx,

        Sadd,
        Scard,
        Sdiff,
        Sdiffstore,
        Sinter,
        Sinterstore,
        Sismember,
        Smembers,
        Smove,
        Spop,
        Srandmember,
        Srem,
        Sscan,
        Sunion,
        Sunionstore,

        Zadd,
        Zcard,
        Zcount,
        Zincrby,
        Zinterstore,
        Zlexcount,
        Zpopmax,
        Zpopmin,
        Zrange,
        Zrangebylex,
        Zrangebyscore,
        Zrank,
        Zrem,
        Zremrangebylex,
        Zremrangebyrank,
        Zremrangebyscore,
        Zrevrange,
        Zrevrangebylex,
        Zrevrangebyscore,
        Zrevrank,
        Zscan,
        Zscore,
        Zunionstore,

        Pfadd,
        Pfcount,
        Pfmerge,

        Geoadd,
        Geodist,
        Geohash,
        Geopos,
        Georadius,
        Georadiusbymember,

        Psubscribe,
        Publish,
        Pubsub,
        Punsubscribe,
        Subscribe,
        Unsubscribe,
        SubMsg,

        MaxCommands,
        MaxCustomCommands = 16,
        AvailableCommands = MaxCommands + MaxCustomCommands,
    };
}
#endif //PUMP_REDIS_COMMAND_NAME_HH
