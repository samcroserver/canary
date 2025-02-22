/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2022 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.org/
*/

#ifndef SRC_ITEMS_TILE_H_
#define SRC_ITEMS_TILE_H_

#include "items/cylinder.h"
#include "declarations.hpp"
#include "items/item.h"
#include "utils/tools.h"

class Creature;
class Teleport;
class TrashHolder;
class Mailbox;
class MagicField;
class QTreeLeafNode;
class BedItem;

using CreatureVector = std::vector<Creature*>;
using ItemVector = std::vector<Item*>;
using SpectatorHashSet = phmap::flat_hash_set<Creature*>;

class TileItemVector : private ItemVector
{
	public:
		using ItemVector::begin;
		using ItemVector::end;
		using ItemVector::rbegin;
		using ItemVector::rend;
		using ItemVector::size;
		using ItemVector::clear;
		using ItemVector::at;
		using ItemVector::insert;
		using ItemVector::erase;
		using ItemVector::push_back;
		using ItemVector::value_type;
		using ItemVector::iterator;
		using ItemVector::const_iterator;
		using ItemVector::reverse_iterator;
		using ItemVector::const_reverse_iterator;
    using ItemVector::empty;

		iterator getBeginDownItem() {
			return begin();
		}
		const_iterator getBeginDownItem() const {
			return begin();
		}
		iterator getEndDownItem() {
			return begin() + downItemCount;
		}
		const_iterator getEndDownItem() const {
			return begin() + downItemCount;
		}
		iterator getBeginTopItem() {
			return getEndDownItem();
		}
		const_iterator getBeginTopItem() const {
			return getEndDownItem();
		}
		iterator getEndTopItem() {
			return end();
		}
		const_iterator getEndTopItem() const {
			return end();
		}

		uint32_t getTopItemCount() const {
			return size() - downItemCount;
		}
		uint32_t getDownItemCount() const {
			return downItemCount;
		}
		inline Item* getTopTopItem() const {
			if (getTopItemCount() == 0) {
				return nullptr;
			}
			return *(getEndTopItem() - 1);
		}
		inline Item* getTopDownItem() const {
			if (downItemCount == 0) {
				return nullptr;
			}
			return *getBeginDownItem();
		}
		void increaseDownItemCount() {
			downItemCount += 1;
		}
		void decreaseDownItemCount() {
			downItemCount -= 1;
		}

	private:
		uint32_t downItemCount = 0;
};

class Tile : public Cylinder
{
	public:
		static Tile& nullptr_tile;
		Tile(uint16_t x, uint16_t y, uint8_t z) : tilePos(x, y, z) {}
		virtual ~Tile() {
			delete ground;
		};

		// non-copyable
		Tile(const Tile&) = delete;
		Tile& operator=(const Tile&) = delete;

		virtual TileItemVector* getItemList() = 0;
		virtual const TileItemVector* getItemList() const = 0;
		virtual TileItemVector* makeItemList() = 0;

		virtual CreatureVector* getCreatures() = 0;
		virtual const CreatureVector* getCreatures() const = 0;
		virtual CreatureVector* makeCreatures() = 0;

		int32_t getThrowRange() const override final {
			return 0;
		}
		bool isPushable() const override final {
			return false;
		}

		MagicField* getFieldItem() const;
		Teleport* getTeleportItem() const;
		TrashHolder* getTrashHolder() const;
		Mailbox* getMailbox() const;
		BedItem* getBedItem() const;

		Creature* getTopCreature() const;
		const Creature* getBottomCreature() const;
		Creature* getTopVisibleCreature(const Creature* creature) const;
		const Creature* getBottomVisibleCreature(const Creature* creature) const;
		Item* getTopTopItem() const;
		Item* getTopDownItem() const;
		bool isMoveableBlocking() const;
		Thing* getTopVisibleThing(const Creature* creature);
		Item* getItemByTopOrder(int32_t topOrder);

		size_t getThingCount() const {
			size_t thingCount = getCreatureCount() + getItemCount();
			if (ground) {
				thingCount++;
			}
			return thingCount;
		}
		// If these return != 0 the associated vectors are guaranteed to exists
		size_t getCreatureCount() const;
		size_t getItemCount() const;
		uint32_t getTopItemCount() const;
		uint32_t getDownItemCount() const;

		bool hasProperty(ItemProperty prop) const;
		bool hasProperty(const Item* exclude, ItemProperty prop) const;

		bool hasFlag(uint32_t flag) const {
			return hasBitSet(flag, this->flags);
		}
		void setFlag(uint32_t flag) {
			this->flags |= flag;
		}
		void resetFlag(uint32_t flag) {
			this->flags &= ~flag;
		}

		ZoneType_t getZone() const {
			if (hasFlag(TILESTATE_PROTECTIONZONE)) {
				return ZONE_PROTECTION;
			} else if (hasFlag(TILESTATE_NOPVPZONE)) {
				return ZONE_NOPVP;
			} else if (hasFlag(TILESTATE_NOLOGOUT)) {
				return ZONE_NOLOGOUT;
			} else if (hasFlag(TILESTATE_PVPZONE)) {
				return ZONE_PVP;
			} else {
				return ZONE_NORMAL;
			}
		}

		bool hasHeight(uint32_t n) const;

		std::string getDescription(int32_t lookDistance) const override final;

		int32_t getClientIndexOfCreature(const Player* player, const Creature* creature) const;
		int32_t getStackposOfCreature(const Player* player, const Creature* creature) const;
		int32_t getStackposOfItem(const Player* player, const Item* item) const;

		//cylinder implementations
		ReturnValue queryAdd(int32_t index, const Thing& thing, uint32_t count,
				uint32_t flags, Creature* actor = nullptr) const override;
		ReturnValue queryMaxCount(int32_t index, const Thing& thing, uint32_t count,
				uint32_t& maxQueryCount, uint32_t flags) const override final;
		ReturnValue queryRemove(const Thing& thing, uint32_t count, uint32_t tileFlags, Creature* actor = nullptr) const override;
		Tile* queryDestination(int32_t& index, const Thing& thing, Item** destItem, uint32_t& flags) override;

		void addThing(Thing* thing) override final;
		void addThing(int32_t index, Thing* thing) override;

		void updateThing(Thing* thing, uint16_t itemId, uint32_t count) override final;
		void replaceThing(uint32_t index, Thing* thing) override final;

		void removeThing(Thing* thing, uint32_t count) override final;

		void removeCreature(Creature* creature);

		int32_t getThingIndex(const Thing* thing) const override final;
		size_t getFirstIndex() const override final;
		size_t getLastIndex() const override final;
		uint32_t getItemTypeCount(uint16_t itemId, int32_t subType = -1) const override final;
		Thing* getThing(size_t index) const override final;

		void postAddNotification(Thing* thing, const Cylinder* oldParent, int32_t index, CylinderLink_t link = LINK_OWNER) override final;
		void postRemoveNotification(Thing* thing, const Cylinder* newParent, int32_t index, CylinderLink_t link = LINK_OWNER) override final;

		void internalAddThing(Thing* thing) override;
		void virtual internalAddThing(uint32_t index, Thing* thing) override;

		const Position& getPosition() const override final {
			return tilePos;
		}

		bool isRemoved() const override final {
			return false;
		}

		Item* getUseItem(int32_t index) const;
		Item* getDoorItem() const;

		Item* getGround() const {
			return ground;
		}
		void setGround(Item* item) {
			ground = item;
		}

	private:
		void onAddTileItem(Item* item);
		void onUpdateTileItem(Item* oldItem, const ItemType& oldType, Item* newItem, const ItemType& newType);
		void onRemoveTileItem(const SpectatorHashSet& spectators, const std::vector<int32_t>& oldStackPosVector, Item* item);
		void onUpdateTile(const SpectatorHashSet& spectators);

		void setTileFlags(const Item* item);
		void resetTileFlags(const Item* item);
		bool fieldIsUnharmable() const;

	protected:
		Item* ground = nullptr;
		Position tilePos;
		uint32_t flags = 0;

};

// Used for walkable tiles, where there is high likeliness of
// items being added/removed
class DynamicTile : public Tile
{
		// By allocating the vectors in-house, we avoid some memory fragmentation
		TileItemVector items;
		CreatureVector creatures;

	public:
		DynamicTile(uint16_t x, uint16_t y, uint8_t z) : Tile(x, y, z) {}
		~DynamicTile() {
			for (Item* item : items) {
				item->decrementReferenceCounter();
			}
		}

		// non-copyable
		DynamicTile(const DynamicTile&) = delete;
		DynamicTile& operator=(const DynamicTile&) = delete;

		TileItemVector* getItemList() override {
			return &items;
		}
		const TileItemVector* getItemList() const override {
			return &items;
		}
		TileItemVector* makeItemList() override {
			return &items;
		}

		CreatureVector* getCreatures() override {
			return &creatures;
		}
		const CreatureVector* getCreatures() const override {
			return &creatures;
		}
		CreatureVector* makeCreatures() override {
			return &creatures;
		}
};

// For blocking tiles, where we very rarely actually have items
class StaticTile final : public Tile
{
	// We very rarely even need the vectors, so don't keep them in memory
	std::unique_ptr<TileItemVector> items;
	std::unique_ptr<CreatureVector> creatures;

	public:
		StaticTile(uint16_t x, uint16_t y, uint8_t z) : Tile(x, y, z) {}
		~StaticTile() {
			if (items) {
				for (Item* item : *items) {
					item->decrementReferenceCounter();
				}
			}
		}

		// non-copyable
		StaticTile(const StaticTile&) = delete;
		StaticTile& operator=(const StaticTile&) = delete;

		TileItemVector* getItemList() override {
			return items.get();
		}
		const TileItemVector* getItemList() const override {
			return items.get();
		}
		TileItemVector* makeItemList() override {
			if (!items) {
				items.reset(new TileItemVector);
			}
			return items.get();
		}

		CreatureVector* getCreatures() override {
			return creatures.get();
		}
		const CreatureVector* getCreatures() const override {
			return creatures.get();
		}
		CreatureVector* makeCreatures() override {
			if (!creatures) {
				creatures.reset(new CreatureVector);
			}
			return creatures.get();
		}
};

#endif  // SRC_ITEMS_TILE_H_
