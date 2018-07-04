#ifndef MAIN_CHAIN_HPP
#define MAIN_CHAIN_HPP

#include "core/byte_array/referenced_byte_array.hpp"
#include "core/mutex.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/consensus/proof_of_work.hpp"
#include "ledger/chain/block.hpp"
#include "crypto/fnv.hpp"
#include<map>
#include<set>

// Specialise hash of byte_array
namespace std {

  template <>
  struct hash<fetch::byte_array::ByteArray>
  {
    std::size_t operator()(const fetch::byte_array::ByteArray& k) const
    {
      fetch::crypto::CallableFNV hasher;

      return hasher(k);
    }
  };
}

namespace fetch
{
namespace chain
{

struct Tip
{
  fetch::byte_array::ByteArray    root{"0"};
  uint64_t                        total_weight;
  bool                            loose;
};

class MainChain
{
 public:
  typedef fetch::chain::consensus::ProofOfWork          proof_type;
  typedef BasicBlock<proof_type, fetch::crypto::SHA256> block_type;
  typedef fetch::byte_array::ByteArray                  block_hash;

  MainChain(block_type &genesis)
  {
    // Add genesis to block chain
    genesis.loose()             = false;
    blockChain_[genesis.hash()] = genesis;

    // Create tip
    auto tip = std::shared_ptr<Tip>(new Tip);
    tip->total_weight     = genesis.weight();
    tip->loose            = false;
    tips_[genesis.hash()] = std::move(tip);
    heaviest_             = std::make_pair(genesis.weight(), genesis.hash());
  }

  // Default hard code genesis
  MainChain()
  {
    block_type genesis;
    genesis.UpdateDigest();

    // Add genesis to block chain
    genesis.loose()             = false;
    blockChain_[genesis.hash()] = genesis;

    // Create tip
    auto tip = std::shared_ptr<Tip>(new Tip);
    tip->total_weight     = genesis.weight();
    tip->loose            = false;
    //tips_[genesis.hash()] = std::move(tip);
    tips_[genesis.hash()] = tip;
    heaviest_             = std::make_pair(genesis.weight(), genesis.hash());
  }

  MainChain(MainChain const &rhs)            = delete;
  MainChain(MainChain &&rhs)                 = delete;
  MainChain &operator=(MainChain const &rhs) = delete;
  MainChain &operator=(MainChain&& rhs)      = delete;

  bool AddBlock(block_type &block)
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);

    // First check if block already exists
    if(blockChain_.find(block.hash()) != blockChain_.end())
    {
      fetch::logger.Warn("Mainchain: Trying to add already seen block");
      return false;
    }

    // Check whether blocks prev hash refers to a valid tip (common case)
    std::shared_ptr<Tip> tip = std::make_shared<Tip>();
    auto tipRef = tips_.find(block.body().previous_hash);

    if(tipRef != tips_.end())
    {
      fetch::logger.Info("Mainchain: Pushing block onto already existing tip");
      // Advance that tip and put block into blockchain
      //tip = std::move((*tipRef).second);
      tip = ((*tipRef).second);
      tips_.erase(tipRef);
      block.loose() = tip->loose;
      tip->total_weight += block.weight();
      block.totalWeight() = tip->total_weight;

      // Blocks need to know if their root is not genesis
      if(tip->loose)
      {
        std::cout << "Expired? " << tip.use_count() << std::endl;
        std::cout << "Tip is " << ToHex(((*tipRef).second)->root) << std::endl;
        block.root() = ((*tipRef).second)->root;
      }

      // Update heaviest pointer if necessary
      if(tip->loose == false && tip->total_weight > heaviest_.first)
      {
        fetch::logger.Info("Updating heaviest with tip");
        heaviest_.first = tip->total_weight;
        heaviest_.second = block.hash();
      }
    }
    else
    {
      // We are not building on a tip, create a new tip
      fetch::logger.Info("Mainchain: Creating new tip");
      auto blockRef = blockChain_.find(block.body().previous_hash);
      tip = std::shared_ptr<Tip>(new Tip);

      // Tip points to existing block
      if(blockRef != blockChain_.end())
      {
        tip->total_weight     = block.weight() + (*blockRef).second.weight();
        block.totalWeight()   = block.weight() + (*blockRef).second.weight();
        tip->loose            = (*blockRef).second.loose();

        if((*blockRef).second.loose())
        {
          block.root()   = (*blockRef).second.root();
        }

        if(tip->loose == false && tip->total_weight > heaviest_.first)
        {
          fetch::logger.Info("Mainchain: creating new tip that is now heaviest! (new fork)");
          heaviest_.first = tip->total_weight;
          heaviest_.second = block.hash();
        }
      }
      else
      {
        fetch::logger.Info("Mainchain: new loose block");
        // we have a block that doesn't refer to anything
        tip->root           = block.body().previous_hash;
        tip->loose          = true;
        block.root()        = block.body().previous_hash;
        block.loose()       = true;
        danglingRoot_[block.body().previous_hash].insert(block.hash());
      }
    }

    // Every time we add a new block there is the possibility this will go on the bottom of a branch
    auto it = danglingRoot_.find(block.hash());
    if(it != danglingRoot_.end())
    {
      fetch::logger.Info("Mainchain: This block completes a dangling root!");
      // Don't want to create a new tip if we are setting a root
      tip.reset();
      std::set<block_hash> &waitingTips = (*it).second;
      fetch::logger.Info("Mainchain: Number of dangling tips: ", waitingTips.size());

      // We have seen that people are looking for this block. Update them.
      for(auto &tipHash : waitingTips)
      {
        // TODO: (`HUT`) : in case of main chain, delete memory mgmt
        fetch::logger.Info("Mainchain: Walking down from tip: ", ToHex(tipHash));
        std::cout << "prev hash is: " << ToHex(block.body().previous_hash) << std::endl;
        std::cout << "prev tiphash is: " << ToHex(tips_[tipHash]->root) << std::endl;
        // Update tip
        tips_[tipHash]->root = block.body().previous_hash;
        tips_[tipHash]->loose = block.loose();

        //// Update block tip points to // TODO: (`HUT`) : delete
        //auto &tipBlock = blockChain_[tipHash].second;
        //tipBlock->root = block.body().previous_hash;
        //tipBlock->loose = block.loose();

        fetch::logger.Info("Setting the block to be loose? ", block.loose());

        block_hash hash = tipHash;

        // Walk down the tree from the tip updating each block to let it know its root
        while(true)
        {
          std::cout << "setting walk hash " << ToHex(hash) << std::endl;
          auto it = blockChain_.find(hash);
          if(it == blockChain_.end())
          {
            std::cout << "not found." << ToHex(hash) << std::endl;
            break;
          }

          block_type &walkBlock = (*it).second;
          if(walkBlock.loose() == false || walkBlock.root() == block.body().previous_hash)
          {
            std::cout << "found bottom of walk: " << walkBlock.loose() << std::endl;
            break;
          }

          walkBlock.totalWeight() = walkBlock.totalWeight() + block.totalWeight();
          walkBlock.loose()       = block.loose();
          walkBlock.root()        = block.body().previous_hash;
          hash                    = walkBlock.body().previous_hash;
        }
      }

      if(block.loose())
      {
        // TODO: (`HUT`) : std::move
        danglingRoot_[block.body().previous_hash] = waitingTips;
      }

      // We need to update the dangling root
      danglingRoot_.erase(it);
    }

    if(tip)
    {
      std::cout << "pushing new tip on! " << std::endl;
      //tips_[block.hash()] = std::move(tip);
      tips_[block.hash()] = tip;
    }
    blockChain_[block.hash()] = block;

    return true;
  }

  block_type const &HeaviestBlock() const
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
    return blockChain_.at(heaviest_.second);
  }

  uint64_t weight() const
  {
    return heaviest_.first;
  }

  std::size_t totalBlocks() const
  {
    return blockChain_.size();
  }

  // TODO: (`HUT`) : callbacks

  // for debugging: get the heaviest chain
  std::vector<block_type> HeaviestChain()
  {
    std::vector<block_type> result;
    std::cout << "walking a1" << std::endl;

    auto topBlock =  blockChain_.at(heaviest_.second);
    std::cout << "walking a2" << std::endl;

    while(topBlock.body().block_number != 0)
    {
    std::cout << "walking a3" << std::endl;
      result.push_back(topBlock);
    std::cout << "walking a4" << std::endl;

      auto hash = topBlock.body().previous_hash;
    std::cout << "walking a5" << std::endl;

      // Walk down
      auto it = blockChain_.find(hash);
      if(it == blockChain_.end())
      {
        fetch::logger.Info("Failed while walking down from top block to find genesis!");
        break;
      }
      std::cout << "walking a6" << std::endl;

      topBlock = (*it).second;
    }

    std::cout << "walking returning!" << std::endl;

    result.push_back(topBlock); // this should be genesis
    std::cout << "walking returning!!" << std::endl;
    return result;
  }

  void reset()
  {
    std::cout << "resetting...\n\n\n\n" << std::endl;
    std::lock_guard<fetch::mutex::Mutex>                 lock(mutex_);
    blockChain_.clear();
    //tips_.clear();
    tips_ = std::unordered_map<block_hash, std::shared_ptr<Tip>>{};
    danglingRoot_.clear();

    // recreate genesis
    block_type genesis;
    genesis.UpdateDigest();

    // Add genesis to block chain
    genesis.loose()             = false;
    blockChain_[genesis.hash()] = genesis;

    // Create tip
    auto tip = std::shared_ptr<Tip>(new Tip);
    tip->total_weight     = genesis.weight();
    tip->loose            = false;
    tips_[genesis.hash()] = std::move(tip);
    heaviest_             = std::make_pair(genesis.weight(), genesis.hash());
    std::cout << "reset." << std::endl;
  }

  bool Get(block_hash hash, block_type &block)
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);

    auto it = blockChain_.find(hash);

    if(it != blockChain_.end())
    {
      block = (*it).second;

      std::cout << "remote000.hash      " << ToHex(block.hash()) << std::endl;
      std::cout << "remote000.prev hash " << ToHex(block.body().previous_hash) << std::endl;
      std::cout << "remote000.Block number " << block.body().block_number << std::endl;

      return true;
    }

    return false;
  }

private:
  std::unordered_map<block_hash, block_type>           blockChain_;
  std::unordered_map<block_hash, std::shared_ptr<Tip>> tips_;
  std::unordered_map<block_hash, std::set<block_hash>> danglingRoot_; // TODO: (`HUT`) : not updated it seems
  std::pair<uint64_t, block_hash>                      heaviest_;
  mutable fetch::mutex::Mutex                          mutex_;
};

}
}


#endif
