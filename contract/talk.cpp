#include <eosio/eosio.hpp>

// Message table
struct [[eosio::table("message"), eosio::contract("talk")]] message {
    uint64_t    id       = {}; // Non-0
    uint64_t    reply_to = {}; // Non-0 if this is a reply
    eosio::name user     = {};
    std::string content  = {};
    uint64_t    likes    = 0;

    uint64_t primary_key() const { return id; }
    uint64_t get_reply_to() const { return reply_to; }
};

using message_table = eosio::multi_index<
    "message"_n, message, eosio::indexed_by<"by.reply.to"_n, eosio::const_mem_fun<message, uint64_t, &message::get_reply_to>>>;

struct [[eosio::table("likes"), eosio::contract("talk")]] likes {
    uint64_t    id       = {}; // Non-0
    std::vector<eosio::name> likedby = std::vector<eosio::name>{};

    uint64_t primary_key() const { return id; }
};

using likes_table = eosio::multi_index<
    "likes"_n, likes>;

// The contract
class talk : eosio::contract {
  public:
    // Use contract's constructor
    using contract::contract;

    // Post a message
    [[eosio::action]] void post(uint64_t id, uint64_t reply_to, eosio::name user, const std::string& content) {
        message_table table{get_self(), 0};
        likes_table likestable{get_self(), 0};

        // Check user
        require_auth(user);

        // Check reply_to exists
        if (reply_to)
            table.get(reply_to);

        // Create an ID if user didn't specify one
        eosio::check(id < 1'000'000'000ull, "user-specified id is too big");
        if (!id)
            id = std::max(table.available_primary_key(), 1'000'000'000ull);

        // Record the message
        table.emplace(get_self(), [&](auto& message) {
            message.id       = id;
            message.reply_to = reply_to;
            message.user     = user;
            message.content  = content;
            message.likes    = 0;
        });

        likestable.emplace(get_self(), [&](auto& likes) {
            likes.id = id;
            likes.likedby = std::vector<eosio::name>{};
        });
    }

    // Like a message
    [[eosio::action]] void like(uint64_t id, eosio::name user) {
        message_table table{get_self(), 0};
        likes_table likestable{get_self(), 0};

        // Check user
        require_auth(user);

        // Create an ID if user didn't specify one
        eosio::check(id < 1'000'000'000ull, "user-specified id is too big");

        auto itr = table.find(id);
        eosio::check(itr != table.end(), "message does not exist!");

        auto likesitr = likestable.find(id);
        eosio::check(likesitr != likestable.end(), "message does not exist in likes_table!");

        // add like the message
        table.modify(itr, get_self(), [&](auto& message) {
            message.likes    += 1;
        });

        likestable.modify(likesitr, get_self(), [&](auto& row) {
            row.likedby.push_back(user);
        });
    }
};
