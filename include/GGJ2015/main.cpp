#include "../GGJ2015/Common.hpp"
#include "../GGJ2015/Boilerplate.hpp"

namespace ggj
{
	namespace Impl
	{
		struct Assets
		{
			ssvs::AssetManager assetManager;

			// Audio players
			ssvs::SoundPlayer soundPlayer;
			ssvs::MusicPlayer musicPlayer;

			// BitmapFonts
			ssvs::BitmapFont* obStroked{nullptr};
			ssvs::BitmapFont* obBig{nullptr};

			// Textures
			sf::Texture* slotChoice{nullptr};

			sf::Texture* iconHPS{nullptr};
			sf::Texture* iconATK{nullptr};
			sf::Texture* iconDEF{nullptr};

			sf::Texture* drops{nullptr};

			sf::Texture* blocked{nullptr};
			sf::Texture* back{nullptr};

			sf::Texture* dropsModal{nullptr};
			sf::Texture* advance{nullptr};

			sf::Texture* itemCard{nullptr};

			inline Assets()
			{
				ssvs::loadAssetsFromJson(assetManager, "Data/", ssvj::Val::fromFile("Data/assets.json"));

				obStroked = &assetManager.get<ssvs::BitmapFont>("fontObStroked");
				obBig = &assetManager.get<ssvs::BitmapFont>("fontObBig");

				slotChoice = &assetManager.get<sf::Texture>("slotChoice.png");

				iconHPS = &assetManager.get<sf::Texture>("iconHPS.png");
				iconATK = &assetManager.get<sf::Texture>("iconATK.png");
				iconDEF = &assetManager.get<sf::Texture>("iconDEF.png");

				drops = &assetManager.get<sf::Texture>("drops.png");

				blocked = &assetManager.get<sf::Texture>("blocked.png");
				back = &assetManager.get<sf::Texture>("back.png");

				dropsModal = &assetManager.get<sf::Texture>("dropsModal.png");
				advance = &assetManager.get<sf::Texture>("advance.png");

				itemCard = &assetManager.get<sf::Texture>("itemCard.png");
			}
		};
	}

	inline auto& getAssets()
	{
		static Impl::Assets result;
		return result;
	}

	inline auto& getEventLogStream()
	{
		static std::stringstream result;
		return result;
	}

	namespace Impl
	{
		struct EventLog
		{
			template<typename T> inline auto operator<<(const T& mX)
			{
				getEventLogStream() << mX;
				ssvu::lo() << mX;
				return EventLog{};
			}
		};
	}

	inline auto eventLo() noexcept
	{
		return Impl::EventLog{};
	}

	using StatType = int;
	using HPS = StatType;
	using ATK = StatType;
	using DEF = StatType;

	struct Constants
	{
		static constexpr SizeT elementCount{4};
		static constexpr float bonusMultiplier{1.5f};
		static constexpr float malusMultiplier{0.8f};
		static constexpr int maxChoices{4};
		static constexpr int maxDrops{3};
	};

	namespace Impl
	{
		class ElementType
		{
			private:
				std::string name;

			public:
				inline ElementType(const std::string& mName) : name{mName}
				{

				}
		};

		class ElementTypeData
		{
			private:
				std::vector<ElementType> elementTypes;

			public:
				inline ElementTypeData()
				{
					elementTypes.emplace_back("Fire");
					elementTypes.emplace_back("Water");
					elementTypes.emplace_back("Earth");
					elementTypes.emplace_back("Lightning");

					SSVU_ASSERT(elementTypes.size() == Constants::elementCount);
				}

				inline const auto& getElementTypes()
				{
					return elementTypes;
				}
		};
	}

	class HardcodedData
	{
		public:
			inline const auto& getElementTypeData()
			{
				static Impl::ElementTypeData result;
				return result;
			}
	};

	using ElementBitset = std::bitset<Constants::elementCount>;

	struct Weapon
	{
		std::string name{"Unarmed"};
		ElementBitset strongAgainst;
		ElementBitset weakAgainst;
		ATK atk{-1};
	};

	struct Armor
	{
		std::string name{"Unarmored"};
		ElementBitset elementTypes;
		DEF def{-1};
	};

	struct Calculations
	{
		inline static bool isWeaponStrongAgainst(const Weapon& mW, const Armor& mA)
		{
			return (mW.strongAgainst & mA.elementTypes).any();
		}

		inline static bool isWeaponWeakAgainst(const Weapon& mW, const Armor& mA)
		{
			return (mW.weakAgainst & mA.elementTypes).any();
		}

		inline static auto getWeaponDamageAgainst(const Weapon& mW, const Armor& mA, ATK mBonusATK, DEF mBonusDEF)
		{
			auto result((mW.atk + mBonusATK) - (mA.def + mBonusDEF));
			if(isWeaponStrongAgainst(mW, mA)) result *= Constants::bonusMultiplier;
			if(isWeaponWeakAgainst(mW, mA)) result *= Constants::malusMultiplier;
			return ssvu::getClampedMin(result, 0);
		}

		inline static bool canWeaponDamage(const Weapon& mW, const Armor& mA, ATK mBonusATK, DEF mBonusDEF)
		{
			return getWeaponDamageAgainst(mW, mA, mBonusATK, mBonusDEF) > 0;
		}
	};

	struct Creature
	{
		std::string name{"Unnamed"};
		Weapon weapon;
		Armor armor;
		HPS hps{-1};

		ATK bonusATK{0};
		DEF bonusDEF{0};

		inline void attackOnce(Creature& mX)
		{
			auto dmg(Calculations::getWeaponDamageAgainst(weapon, mX.armor, bonusATK, mX.bonusDEF));
			mX.hps -= dmg;

			eventLo() << name << " hits " << mX.name << " for " << dmg << " dmg!\n";

			if(Calculations::isWeaponStrongAgainst(weapon, mX.armor))
				eventLo() << "(Strong attack!)\n";

			if(Calculations::isWeaponWeakAgainst(weapon, mX.armor))
				eventLo() << "(Weak attack!)\n";

			if(mX.isDead())
				eventLo() << mX.name << " is dead!\n";

			eventLo() << "\n";
		}

		inline void fight(Creature& mX)
		{
			eventLo() << name << " engages " << mX.name << "!\n";

			while(true)
			{
				attackOnce(mX);
				if(mX.isDead()) break;

				mX.attackOnce(*this);
				if(isDead()) break;
			}
		}

		inline bool canDamage(Creature& mX) const noexcept
		{
			return Calculations::canWeaponDamage(weapon, mX.armor, bonusATK, mX.bonusDEF);
		}

		inline bool isDead() const noexcept { return hps <= 0; }

		inline std::string getLogStr() const
		{
			std::string result;

			result += "HPS: " + ssvu::toStr(hps) + ", ";
			result += "ATK: " + ssvu::toStr(weapon.atk) + ", ";
			result += "DEF: " + ssvu::toStr(armor.def) + ", ";
			result += "Str: " + ssvu::toStr(weapon.strongAgainst) + ", ";
			result += "Wkk: " + ssvu::toStr(weapon.weakAgainst);

			return result;
		}
	};

	namespace Impl
	{
		struct NameGenData
		{
			float chance;
			std::string str;

			inline NameGenData(float mChance, const std::string& mStr)
				: chance{mChance}, str{mStr}
			{

			}
		};

		struct Gen
		{
			inline const auto& getWeapons()
			{
				static std::vector<NameGenData> result
				{
					{1.0f,		"Sword"},
					{1.0f,		"Spear"},
					{1.0f,		"Staff"},
					{1.0f,		"Gauntlet"},
					{1.0f,		"Wand"},
					{0.8f,		"Greatsword"},
					{0.8f,		"Claymore"},
					{0.7f,		"Magical sword"},
					{0.7f,		"Enchanted gauntlets"},
					{0.5f,		"Greatstaff"},
				};

				return result;
			}

			inline const auto& getItemModifiers()
			{
				static std::vector<NameGenData> result
				{
					{1.0f,		"Rusty"},
					{1.0f,		"Damaged"},
					{1.0f,		"Dented"},
					{1.0f,		"Regular"},
					{0.8f,		"Powerful"},
					{0.8f,		"Intense"},
					{0.8f,		"Heavy"},
					{0.7f,		"Incredible"},
					{0.7f,		"Excellent"},
					{0.5f,		"Supreme"},
				};

				return result;
			}

			inline const auto& getCreatures()
			{
				static std::vector<NameGenData> result
				{
					{1.0f,		"Slime"},
					{1.0f,		"Skeleton"},
					{1.0f,		"Dragonkin"},
					{1.0f,		"Giant crab"},
					{0.8f,		"Undead"},
					{0.8f,		"Zombie"},
					{0.8f,		"Dragon"},
					{0.7f,		"Ghost"},
					{0.7f,		"Bloodkin"},
					{0.5f,		"Vampire"},
				};

				return result;
			}

			inline const auto& getCreatureModifier()
			{
				static std::vector<NameGenData> result
				{
					{1.0f,		"Injured"},
					{1.0f,		"Diseased"},
					{1.0f,		"Enraged"},
					{1.0f,		"Powerful"},
					{0.8f,		"Undead"},
					{0.8f,		"Magical"},
					{0.8f,		"Enchanted"},
					{0.7f,		"Phantasm"},
					{0.7f,		"Bloodthirsty"},
					{0.5f,		"Ravaging"},
				};

				return result;
			}

			template<typename T> inline const auto& getR(const T& mX)
			{
				float weightSum{0.f};
				for(const auto& x : mX) weightSum += x.chance;
				auto r(ssvu::getRndR(0.f, weightSum));
				auto t(0.f);

				for(const auto& x : mX)
				{
					t += x.chance;
					if(t > r) return x.str;
				}

				return mX[ssvu::getRnd(0ul, mX.size())].str;
			}

			template<typename T> inline void whileChance(int mChance, const T& mFn)
			{
				while(ssvu::getRnd(0, 100) < mChance)
				{
					mFn();
					mChance /= 2;
					if(mChance < 2) mChance = 2;
				}
			}

			inline auto generateWeaponName()
			{
				std::string result;
				return result;
			}

			inline auto generateCreatureName()
			{
				std::string result;

				whileChance(25, [this, &result]{ result += getR(getCreatureModifier()) + " "; });
				result += getR(getCreatures());

				return result;
			}
		};
	}

	inline auto& getGen()
	{
		static Impl::Gen result;
		return result;
	}

	struct InstantEffect
	{
		enum class Type
		{
			Add,
			Sub,
			Mul,
			Div
		};

		enum class Stat
		{
			SHPS,
			SATK,
			SDEF
		};

		Type type;
		Stat stat;
		StatType value;

		inline InstantEffect(Type mType, Stat mStat, StatType mValue) : type{mType}, stat{mStat}, value{mValue} { }
		inline void apply(Creature& mX)
		{
			StatType* statPtr{nullptr};

			switch(stat)
			{
				case Stat::SHPS: statPtr = &mX.hps; break;
				case Stat::SATK: statPtr = &mX.bonusATK; break;
				case Stat::SDEF: statPtr = &mX.bonusDEF; break;
			}

			switch(type)
			{
				case Type::Add: *statPtr += value; break;
				case Type::Sub: *statPtr -= value; ssvu::clampMin(*statPtr, 1); break;
				case Type::Mul: *statPtr *= value; break;
				case Type::Div: *statPtr /= value; ssvu::clampMin(*statPtr, 1); break;
			}
		}

		inline std::string getStrType()
		{
			switch(type)
			{
				case Type::Add: return "Add";
				case Type::Sub: return "Subtract";
				case Type::Mul: return "Multiply";
				case Type::Div: return "Divide";
			}
		}

		inline std::string getStrStat()
		{
			switch(stat)
			{
				case Stat::SHPS: return "HPS";
				case Stat::SATK: return "ATK";
				case Stat::SDEF: return "DEF";
			}
		}
	};

	struct Drop
	{
		sf::Sprite card;

		inline Drop()
		{
			card.setTexture(*getAssets().itemCard);
			card.setOrigin(Vec2f{card.getTexture()->getSize()} / 2.f);
		}

		inline virtual ~Drop() { }
		inline virtual void apply(Creature&) { }

		inline virtual void draw(ssvs::GameWindow& mGW, const Vec2f&, const Vec2f& mCenter)
		{
			card.setPosition(mCenter + Vec2f{0, -20.f});
			mGW.draw(card);
		}
	};

	struct DropIE : public Drop
	{
		InstantEffect ie;
		ssvs::BitmapText txtType{*getAssets().obStroked, ""};
		ssvs::BitmapText txtValue{*getAssets().obStroked, ""};
		ssvs::BitmapText txtStat{*getAssets().obStroked, ""};

		inline DropIE(const InstantEffect& mIE) : ie{mIE}
		{
			txtType.setString(ie.getStrType());
			txtValue.setString(ssvu::toStr(ie.value));
			txtStat.setString(ie.getStrStat());

			txtType.setTracking(-3);
			txtValue.setTracking(-3);
			txtStat.setTracking(-3);

			txtType.setOrigin(ssvs::getGlobalHalfSize(txtType));
			txtValue.setOrigin(ssvs::getGlobalHalfSize(txtValue));
			txtStat.setOrigin(ssvs::getGlobalHalfSize(txtStat));
		}

		inline void apply(Creature& mX) override
		{
			ie.apply(mX);
		}

		inline void draw(ssvs::GameWindow& mGW, const Vec2f& mPos, const Vec2f& mCenter) override
		{
			Drop::draw(mGW, mPos, mCenter);

			txtType.setPosition(card.getPosition() + Vec2f{0, -10.f});
			txtValue.setPosition(card.getPosition() + Vec2f{0, 0.f});
			txtStat.setPosition(card.getPosition() + Vec2f{0, 10.f});

			mGW.draw(txtType);
			mGW.draw(txtValue);
			mGW.draw(txtStat);
		}
	};

	struct ItemDrops
	{
		ssvu::UPtr<Drop> drops[Constants::maxDrops];

		inline ItemDrops()
		{
			for(auto i(0u); i < Constants::maxDrops; ++i)
				drops[i] = nullptr;


		}

		inline bool has(int mIdx)
		{
			return drops[mIdx] != nullptr;
		}

		inline void give(int mIdx, Creature& mX)
		{
			drops[mIdx]->apply(mX);
			drops[mIdx].release();
		}
	};

	class GameSession;

	struct Choice
	{
		GameSession& gameSession;
		SizeT idx;

		inline Choice(GameSession& mGameState, SizeT mIdx) : gameSession{mGameState}, idx{mIdx} { }
		inline virtual ~Choice() { }

		inline virtual void execute() { }
		inline virtual void draw(ssvs::GameWindow&, const Vec2f&, const Vec2f&) { }

		inline virtual std::string getChoiceStr() { return ""; }
	};

	struct ChoiceAdvance : public Choice
	{
		sf::Sprite advanceSprite;

		inline ChoiceAdvance(GameSession& mGameState, SizeT mIdx) : Choice{mGameState, mIdx}
		{
			advanceSprite.setTexture(*getAssets().advance);
			advanceSprite.setOrigin(Vec2f{advanceSprite.getTexture()->getSize()} / 2.f);
		}

		inline void execute() override;
		inline void draw(ssvs::GameWindow&, const Vec2f&, const Vec2f&) override;

		inline std::string getChoiceStr() override
		{
			return "Forward";
		}
	};

	struct CreatureStatsDraw
	{
		sf::Sprite iconHPS;
		sf::Sprite iconATK;
		sf::Sprite iconDEF;
		ssvs::BitmapText txtHPS;
		ssvs::BitmapText txtATK;
		ssvs::BitmapText txtDEF;

		inline CreatureStatsDraw()
			: txtHPS{*getAssets().obStroked, ""}, txtATK{*getAssets().obStroked, ""}, txtDEF{*getAssets().obStroked, ""}
		{
			iconHPS.setTexture(*getAssets().iconHPS);
			iconATK.setTexture(*getAssets().iconATK);
			iconDEF.setTexture(*getAssets().iconDEF);

			txtHPS.setTracking(-3);
			txtATK.setTracking(-3);
			txtDEF.setTracking(-3);
		}

		inline void draw(Creature& mC, ssvs::GameWindow& mGW, const Vec2f& mPos, const Vec2f&)
		{
			iconHPS.setPosition(mPos + Vec2f{0.f, 12.f * 0.f});
			iconATK.setPosition(mPos + Vec2f{0.f, 12.f * 1.f});
			iconDEF.setPosition(mPos + Vec2f{0.f, 12.f * 2.f});

			txtHPS.setPosition(mPos + Vec2f{12.f, 12.f * 0.f});
			txtATK.setPosition(mPos + Vec2f{12.f, 12.f * 1.f});
			txtDEF.setPosition(mPos + Vec2f{12.f, 12.f * 2.f});

			txtHPS.setString(ssvu::toStr(mC.hps));
			txtATK.setString(ssvu::toStr(mC.weapon.atk) + " (+"+ ssvu::toStr(mC.bonusATK) + ")");
			txtDEF.setString(ssvu::toStr(mC.armor.def) + " (+" + ssvu::toStr(mC.bonusDEF) + ")");

			mGW.draw(iconHPS);
			mGW.draw(iconATK);
			mGW.draw(iconDEF);

			mGW.draw(txtHPS);
			mGW.draw(txtATK);
			mGW.draw(txtDEF);
		}
	};

	struct ChoiceCreature : public Choice
	{
		Creature creature;
		CreatureStatsDraw csd;

		inline ChoiceCreature(GameSession& mGameState, SizeT mIdx)
			: Choice{mGameState, mIdx}
		{
			csd.iconHPS.setTexture(*getAssets().iconHPS);
			csd.iconATK.setTexture(*getAssets().iconATK);
			csd.iconDEF.setTexture(*getAssets().iconDEF);

			csd.txtHPS.setTracking(-3);
			csd.txtATK.setTracking(-3);
			csd.txtDEF.setTracking(-3);
		}

		inline void execute() override;
		inline void draw(ssvs::GameWindow&, const Vec2f&, const Vec2f&) override;

		inline std::string getChoiceStr() override
		{
			return "Fight";
		}
	};

	struct ChoiceItemDrop : public Choice
	{
		sf::Sprite drops;
		ItemDrops itemDrops;

		ChoiceItemDrop(GameSession& mGS, SizeT mIdx);

		inline void execute() override;
		inline void draw(ssvs::GameWindow&, const Vec2f&, const Vec2f&) override;

		inline std::string getChoiceStr() override
		{
			return "Collect";
		}
	};

	struct GameSession
	{
		int roomNumber{0};
		Creature player;
		ssvu::UPtr<Choice> choices[Constants::maxChoices];
		ssvu::UPtr<Choice> nextChoices[Constants::maxChoices];
		float timer;
		float difficultyMultiplier{1.f};
		float rndMultiplier{1.5f};

		ItemDrops* currentDrops{nullptr};

		inline GameSession()
		{
			Weapon startingWeapon;
			startingWeapon.atk = 10;
			startingWeapon.name = "Starting weapon";

			Armor startingArmor;
			startingArmor.def = 5;
			startingArmor.name = "Starting armor";

			player.name = "Player";
			player.hps = 100;
			player.weapon = startingWeapon;
			player.armor = startingArmor;

			advance();
		}

		inline void tryPickupDrop(int mIdx)
		{
			if(!currentDrops->has(mIdx)) return;

			currentDrops->give(mIdx, player);
		}

		inline void startDrops(ItemDrops* mID)
		{
			currentDrops = mID;
		}
		inline void endDrops()
		{
			currentDrops = nullptr;
		}


		inline void refreshChoices()
		{
			for(auto i(0u); i < Constants::maxChoices; ++i)
			{
				if(nextChoices[i] == nullptr) continue;
				choices[i] = std::move(nextChoices[i]);
				nextChoices[i] = nullptr;
			}
		}

		inline void resetTimer()
		{
			timer = ssvu::getSecondsToFT(10);
		}

		inline void generateRndElements(int mL, ElementBitset& mX)
		{
			auto d(static_cast<int>(mL * difficultyMultiplier));

			for(int i{1}; i < d / 3; ++i)
				if(ssvu::getRnd(0, 100) < 25 * rndMultiplier)
					mX[ssvu::getRnd(0ul, Constants::elementCount)] = true;
		}

		inline int getRndStat(int mL, float mMultMin, float mMultMax)
		{
			auto d(static_cast<int>(mL * difficultyMultiplier));

			return ssvu::getClampedMin(1, d + ssvu::getRnd(static_cast<int>((mMultMin * d) * rndMultiplier), static_cast<int>((mMultMax * d) * rndMultiplier)));
		}

		inline InstantEffect generateInstantEffect(int mL)
		{
			auto d(static_cast<int>(mL * difficultyMultiplier));

			InstantEffect::Stat stat;
			InstantEffect::Type type;

			auto statN = ssvu::getRnd(0, 3);
			if(statN == 0) stat = InstantEffect::Stat::SHPS;
			if(statN == 1) stat = InstantEffect::Stat::SATK;
			if(statN == 2) stat = InstantEffect::Stat::SDEF;

			auto typeN = ssvu::getRnd(0, 4);
			if(typeN == 0) type = InstantEffect::Type::Add;
			if(typeN == 1) type = InstantEffect::Type::Sub;
			if(typeN == 2) type = InstantEffect::Type::Mul;
			if(typeN == 3) type = InstantEffect::Type::Div;

			auto val(getRndStat(mL, -1.5f, 1.5f));
			if(statN == 0) val *= 3;

			InstantEffect result
			{
				type, stat, val
			};

			return result;
		}

		inline ItemDrops generateDrops(int mL)
		{
			auto d(static_cast<int>(mL * difficultyMultiplier));

			ItemDrops result;

			for(auto i(0u); i < Constants::maxDrops; ++i)
			{
				if(ssvu::getRnd(0, 50) > 25) continue;

				auto ie(generateInstantEffect(mL));

				result.drops[i] = ssvu::makeUPtr<DropIE>(ie);
			}

			return result;
		}

		inline Weapon generateWeapon(int mL)
		{
			auto d(static_cast<int>(mL * difficultyMultiplier));

			Weapon result;

			result.name = "Generated name TODO (lvl: " + ssvu::toStr(d) + ")";
			result.atk = getRndStat(mL, -1.5f, 1.8f);
			generateRndElements(mL, result.strongAgainst);
			generateRndElements(mL, result.weakAgainst);

			return result;
		}

		inline Armor generateArmor(int mL)
		{
			auto d(static_cast<int>(mL * difficultyMultiplier));

			Armor result;

			result.name = "Generated name TODO (lvl: " + ssvu::toStr(d) + ")";
			result.def = getRndStat(mL, -1.5f, 1.8f) * 0.7f;
			generateRndElements(mL, result.elementTypes);

			return result;
		}

		inline Creature generateCreature(int mL)
		{
			auto d(static_cast<int>(mL * difficultyMultiplier));

			Creature result;

			result.name = getGen().generateCreatureName();
			result.armor = generateArmor(mL);
			result.weapon = generateWeapon(mL);
			result.hps = d * 5 + ssvu::getRnd(0, d * 3);

			return result;
		}

		inline void generateChoices()
		{
			for(auto i(0u); i < Constants::maxChoices; ++i)
			{
				choices[i].release();

				if(ssvu::getRnd(0, 50) > 25) continue;

				auto choice(ssvu::makeUPtr<ChoiceCreature>(*this, i));
				choice->creature = generateCreature(roomNumber);

				choices[i] = std::move(choice);
			}
		}


		template<typename T> inline void resetChoiceAt(SizeT mIdx, T&& mX)
		{
			nextChoices[mIdx] = ssvu::fwd<T>(mX);
		}

		inline void advance()
		{
			++roomNumber;
			generateChoices();
			resetTimer();
			endDrops();
		}
	};

	inline void ChoiceAdvance::execute()
	{
		gameSession.advance();
	}
	inline void ChoiceAdvance::draw(ssvs::GameWindow& mGW, const Vec2f&, const Vec2f& mCenter)
	{
		advanceSprite.setPosition(mCenter);
		mGW.draw(advanceSprite);
	}

	inline ChoiceItemDrop::ChoiceItemDrop(GameSession& mGS, SizeT mIdx) : Choice{mGS, mIdx}
	{
		drops.setTexture(*getAssets().drops);
		itemDrops = mGS.generateDrops(mGS.roomNumber);
	}
	inline void ChoiceItemDrop::execute()
	{
		gameSession.startDrops(&itemDrops);
		gameSession.resetChoiceAt(idx, ssvu::makeUPtr<ChoiceAdvance>(gameSession, idx));
	}
	inline void ChoiceItemDrop::draw(ssvs::GameWindow& mGW, const Vec2f& mPos, const Vec2f&)
	{
		drops.setPosition(mPos);
		mGW.draw(drops);
	}

	inline void ChoiceCreature::execute()
	{
		if(gameSession.player.canDamage(creature))
		{
			gameSession.player.fight(creature);
			gameSession.resetChoiceAt(idx, ssvu::makeUPtr<ChoiceItemDrop>(gameSession, idx));
		}
		else
		{
			eventLo() << gameSession.player.name << " cannot fight " << creature.name << "!\n";
		}
	}
	inline void ChoiceCreature::draw(ssvs::GameWindow& mGW, const Vec2f& mPos, const Vec2f& mCenter)
	{
		Vec2f offset{4.f, 4.f};
		csd.draw(creature, mGW, offset + mPos, mCenter);
	}

	struct SlotBase
	{

	};

	struct SlotChoice : public SlotBase
	{
		sf::RectangleShape shape;
		sf::Sprite sprite;
		ssvs::BitmapText txtNum;
		ssvs::BitmapText txtStr;
		int choice;

		static constexpr float step{300.f / 4.f};

		inline SlotChoice(int mChoice) : txtNum{*getAssets().obBig, ssvu::toStr(mChoice + 1)},
			txtStr{*getAssets().obStroked, ""}, choice{mChoice}
		{
			shape.setSize(Vec2f{step, 130.f});
			shape.setFillColor(sf::Color::Red);
			shape.setPosition(Vec2f{10 + step * mChoice, 40});

			sprite.setTexture(*getAssets().slotChoice);
			sprite.setPosition(Vec2f{10 + step * mChoice, 40});

			txtNum.setOrigin(ssvs::getGlobalHalfSize(txtNum));
			txtNum.setPosition(Vec2f{10 + step * mChoice + (step / 2.f), 40 + 105});

			txtStr.setTracking(-3);
		}

		inline void update()
		{
			txtStr.setOrigin(ssvs::getGlobalHalfSize(txtStr));
			txtStr.setPosition(Vec2f{10 + step * choice + (step / 2.f), 40 + 120});
		}

		inline Vec2f getCenter()
		{
			return Vec2f{10 + (step * choice) + (step / 2.f), 40 + 130.f / 2.f};
		}

		inline void drawInCenter(ssvs::GameWindow& mGW, const sf::Texture& mX)
		{
			sf::Sprite s;
			s.setTexture(mX);
			s.setOrigin(Vec2f{s.getTexture()->getSize()} / 2.f);
			s.setPosition(getCenter());
			mGW.draw(s);
		}
	};

	class GameApp : public Boilerplate::App
	{
		private:
			GameSession gameSession;
			ssvs::BitmapText tempLog;
			ssvs::BitmapText txtTimer, txtRoom;
			std::vector<SlotChoice> slotChoices;
			sf::Sprite dropsModalSprite;
			CreatureStatsDraw csdPlayer;

			inline void initInput()
			{
				auto& gs(gameState);

				gs.addInput({{IK::Escape}}, [this](FT){ gameWindow->stop(); });

				gs.addInput({{IK::A}}, [this](FT){ gameCamera.pan(-4, 0); });
				gs.addInput({{IK::D}}, [this](FT){ gameCamera.pan(4, 0); });
				gs.addInput({{IK::W}}, [this](FT){ gameCamera.pan(0, -4); });
				gs.addInput({{IK::S}}, [this](FT){ gameCamera.pan(0, 4); });
				gs.addInput({{IK::Q}}, [this](FT){ gameCamera.zoomOut(1.1f); });
				gs.addInput({{IK::E}}, [this](FT){ gameCamera.zoomIn(1.1f); });

				gs.addInput({{IK::Num1}}, [this](FT){ executeChoice(0); }, IT::Once);
				gs.addInput({{IK::Num2}}, [this](FT){ executeChoice(1); }, IT::Once);
				gs.addInput({{IK::Num3}}, [this](FT){ executeChoice(2); }, IT::Once);
				gs.addInput({{IK::Num4}}, [this](FT){ executeChoice(3); }, IT::Once);
			}

			inline void executeChoice(int mI)
			{
				if(gameSession.currentDrops == nullptr)
				{
					if(gameSession.choices[mI] == nullptr) return;

					gameSession.choices[mI]->execute();

					if(gameSession.currentDrops == nullptr)
						gameSession.refreshChoices();
				}
				else
				{
					if(mI == 0)
					{
						gameSession.endDrops();
						gameSession.refreshChoices();
					}
					else
					{
						gameSession.tryPickupDrop(mI - 1);
					}
				}
			}

			inline void update(FT mFT)
			{
				gameCamera.update<float>(mFT);



				gameSession.timer -= mFT;

				if(gameSession.timer <= 0)
				{
					txtTimer.setString("RIP LOL XDddddddddddddddddddddD");
				}
				else
				{
					auto intt(ssvu::getFTToSeconds(static_cast<int>(gameSession.timer)));
					auto gts(intt >= 10 ? ssvu::toStr(intt) : "0" + ssvu::toStr(intt));

					auto third(gameWindow->getWidth() / 5.f);

					txtTimer.setString("00:" + gts);
					txtTimer.setOrigin(ssvs::getGlobalHalfSize(txtTimer));
					txtTimer.setPosition(third * 1.f, 20);

					txtRoom.setString("Room:" + ssvu::toStr(gameSession.roomNumber));
					txtRoom.setOrigin(ssvs::getGlobalHalfSize(txtRoom));
					txtRoom.setPosition(third * 4.f, 20);

					std::string choiceStr;

					int i{0};
					for(auto& c : gameSession.choices)
					{
						if(c == nullptr) continue;

						choiceStr += ssvu::toStr(i) + ". " + c->getChoiceStr() + "\n\n";
						++i;
					}

					auto els(getEventLogStream().str());

					std::string elsLog;

					int foundNewLines{0};

					for(auto itr(els.rbegin()); itr < els.rend(); ++itr)
					{
						elsLog += *itr;
						if(*itr == '\n') ++foundNewLines;
						if(foundNewLines == 5) break;
					}

					tempLog.setString(std::string{elsLog.rbegin(), elsLog.rend()});
				}
			}

			inline void draw()
			{
				gameCamera.apply();
				{

				}
				gameCamera.unapply();

				gameWindow->draw(txtTimer);
				gameWindow->draw(txtRoom);

				if(gameSession.currentDrops != nullptr)
				{
					gameWindow->draw(dropsModalSprite);

					for(auto i(0u); i < slotChoices.size(); ++i)
					{
						auto& sc(slotChoices[i]);

						if(i == 0)
						{
							sc.drawInCenter(*gameWindow, *getAssets().back);
							sc.txtStr.setString("Back");
						}
						else
						{
							if(gameSession.currentDrops->has(i - 1))
							{
								gameSession.currentDrops->drops[i -1]->draw(*gameWindow, sc.shape.getPosition(), sc.getCenter());
								sc.txtStr.setString("Pickup");
							}
						}

						sc.update();

						if(i == 0 || gameSession.currentDrops->has(i - 1))
						{
							gameWindow->draw(sc.txtNum);
							gameWindow->draw(sc.txtStr);
						}
					}
				}
				else
				{
					for(auto i(0u); i < slotChoices.size(); ++i)
					{
						auto& sc(slotChoices[i]);

						if(gameSession.choices[i] != nullptr)
						{
							sc.txtStr.setString(gameSession.choices[i]->getChoiceStr());
						}
						else
						{
							sc.txtStr.setString("Blocked");
						}

						sc.update();

						gameWindow->draw(sc.shape);
						gameWindow->draw(sc.sprite);

						if(gameSession.choices[i] != nullptr)
						{
							gameSession.choices[i]->draw(*gameWindow, sc.shape.getPosition(), sc.getCenter());
						}
						else
						{
							sc.drawInCenter(*gameWindow, *getAssets().blocked);
						}

						gameWindow->draw(sc.txtNum);
						gameWindow->draw(sc.txtStr);
					}
				}

				gameWindow->draw(tempLog);

				csdPlayer.draw(gameSession.player, *gameWindow, Vec2f{10, 175}, Vec2f{0.f, 0.f});
			}

		public:
			inline GameApp(ssvs::GameWindow& mGameWindow)
				: Boilerplate::App{mGameWindow}, tempLog{*getAssets().obStroked, ""}, txtTimer{*getAssets().obBig, ""},
				  txtRoom{*getAssets().obBig, ""}
			{
				tempLog.setTracking(-3);
				txtTimer.setTracking(-1);
				txtRoom.setTracking(-1);

				for(int i{0}; i < 4; ++i)
					slotChoices.emplace_back(i);

				gameState.onUpdate += [this](FT mFT){ update(mFT); };
				gameState.onDraw += [this]{ draw(); };

				dropsModalSprite.setTexture(*getAssets().dropsModal);
				dropsModalSprite.setPosition(10, 40);

				tempLog.setPosition(Vec2f{75, 180});

				initInput();
				//initTest();
			}
	};
}

int main()
{
	Boilerplate::AppRunner<ggj::GameApp>{"GGJ2015", 320, 240};
	return 0;
}

