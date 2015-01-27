#include "../GGJ2015/Common.hpp"
#include "../GGJ2015/Boilerplate.hpp"

// TODO: better resource caching system in SSVS
// TODO: load resources from folder, not json?

#define CACHE_ASSET(mType, mName, mExt) mType* mName{&assetLoader.assetManager.get<mType>(SSVPP_TOSTR(mName) mExt)}

namespace ggj
{
	class GameSession;

	namespace Impl
	{
		struct AssetLoader
		{
			ssvs::AssetManager assetManager;

			inline AssetLoader()
			{
				ssvs::loadAssetsFromJson(assetManager, "Data/", ssvj::Val::fromFile("Data/assets.json"));
			}
		};

		struct Assets
		{
			AssetLoader assetLoader{};

			// Audio players
			ssvs::SoundPlayer soundPlayer;
			ssvs::MusicPlayer musicPlayer;

			// BitmapFonts
			CACHE_ASSET(ssvs::BitmapFont, fontObStroked, "");
			CACHE_ASSET(ssvs::BitmapFont, fontObBig, "");

			// Textures
			CACHE_ASSET(sf::Texture, slotChoice, ".png");
			CACHE_ASSET(sf::Texture, iconHPS, ".png");
			CACHE_ASSET(sf::Texture, iconATK, ".png");
			CACHE_ASSET(sf::Texture, iconDEF, ".png");
			CACHE_ASSET(sf::Texture, drops, ".png");
			CACHE_ASSET(sf::Texture, enemy, ".png");
			CACHE_ASSET(sf::Texture, blocked, ".png");
			CACHE_ASSET(sf::Texture, back, ".png");
			CACHE_ASSET(sf::Texture, dropsModal, ".png");
			CACHE_ASSET(sf::Texture, advance, ".png");
			CACHE_ASSET(sf::Texture, itemCard, ".png");
			CACHE_ASSET(sf::Texture, eFire, ".png");
			CACHE_ASSET(sf::Texture, eWater, ".png");
			CACHE_ASSET(sf::Texture, eEarth, ".png");
			CACHE_ASSET(sf::Texture, eLightning, ".png");
			CACHE_ASSET(sf::Texture, eST, ".png");
			CACHE_ASSET(sf::Texture, eWK, ".png");
			CACHE_ASSET(sf::Texture, eTY, ".png");
			CACHE_ASSET(sf::Texture, equipCard, ".png");
			CACHE_ASSET(sf::Texture, wpnMace, ".png");
			CACHE_ASSET(sf::Texture, wpnSword, ".png");
			CACHE_ASSET(sf::Texture, wpnSpear, ".png");
			CACHE_ASSET(sf::Texture, armDrop, ".png");

			// Sounds
			CACHE_ASSET(sf::SoundBuffer, lvl1, ".wav");
			CACHE_ASSET(sf::SoundBuffer, lvl2, ".wav");
			CACHE_ASSET(sf::SoundBuffer, lvl3, ".wav");
			CACHE_ASSET(sf::SoundBuffer, lvl4, ".wav");
			CACHE_ASSET(sf::SoundBuffer, menu, ".wav");
			CACHE_ASSET(sf::SoundBuffer, powerup, ".wav");
			CACHE_ASSET(sf::SoundBuffer, drop, ".wav");
			CACHE_ASSET(sf::SoundBuffer, grab, ".wav");
			CACHE_ASSET(sf::SoundBuffer, equipArmor, ".wav");
			CACHE_ASSET(sf::SoundBuffer, equipWpn, ".wav");
			CACHE_ASSET(sf::SoundBuffer, lose, ".wav");

			std::vector<sf::SoundBuffer*> swordSnds, maceSnds, spearSnds;

			inline Assets()
			{
				std::vector<std::string> elems{"normal","fire","water","earth","lightning"};

				for(auto& e : elems)
				{
					swordSnds.emplace_back(&assetLoader.assetManager.get<sf::SoundBuffer>("sword/" + e + ".wav"));
					maceSnds.emplace_back(&assetLoader.assetManager.get<sf::SoundBuffer>("mace/" + e + ".wav"));
					spearSnds.emplace_back(&assetLoader.assetManager.get<sf::SoundBuffer>("spear/" + e + ".wav"));
				}

				soundPlayer.setVolume(100.f);
			}
		};
	}

	inline auto& getAssets() noexcept { static Impl::Assets result; return result; }
	inline auto& getEventLogStream() noexcept { static std::stringstream result; return result; }

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

	inline auto eventLo() noexcept { return Impl::EventLog{}; }

	using StatType = int;
	using HPS = StatType;
	using ATK = StatType;
	using DEF = StatType;

	struct Constants
	{
		static constexpr SizeT elementCount{4};
		static constexpr float bonusMultiplier{2.5f};
		static constexpr float malusMultiplier{0.8f};
		static constexpr int maxChoices{4};
		static constexpr int maxDrops{3};
	};

	using ElementBitset = std::bitset<Constants::elementCount>;

	struct Weapon
	{
		enum class Type : int {Mace = 0 , Sword = 1, Spear = 2};

		std::string name{"Unarmed"};
		ElementBitset strongAgainst;
		ElementBitset weakAgainst;
		ATK atk{-1};
		Type type{Type::Mace};

		inline sf::Texture* getTypeTexture()
		{
			switch(type)
			{
				case Type::Mace: return getAssets().wpnMace;
				case Type::Sword: return getAssets().wpnSword;
				case Type::Spear: return getAssets().wpnSpear;
			}

			return getAssets().wpnMace;
		}

		inline void playAttackSounds()
		{
			auto vec(&(getAssets().swordSnds));
			if(type == Type::Mace) vec = &getAssets().maceSnds;
			else if(type == Type::Spear) vec = &getAssets().spearSnds;

			// Normal
			if(strongAgainst.none())
			{
				ssvu::lo() << "Play snd" << std::endl;
				getAssets().soundPlayer.play(*(*vec)[0]);
			}
			else
			{
				for(auto i(0u); i < Constants::elementCount; ++i)
				{
					if(strongAgainst[i]) getAssets().soundPlayer.play(*(*vec)[i + 1]);
				}
			}
		}
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
		}

		void checkBurns(GameSession& mGameSession);

		inline void fight(Creature& mX)
		{
			eventLo() << name << " engages " << mX.name << "!\n";
			auto hpsBefore(hps);
			auto xHPSBefore(mX.hps);

			while(true)
			{
				attackOnce(mX);
				if(mX.isDead()) break;

				mX.attackOnce(*this);
				if(isDead()) break;
			}

			if(isDead())
				eventLo() << mX.name << " wins. HPS " << xHPSBefore << " -> " << mX.hps << "!\n";
			else
				eventLo() << name << " wins. HPS " << hpsBefore << " -> " << hps << "!\n";
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

			inline NameGenData(float mChance, const std::string& mStr) : chance{mChance}, str{mStr} { }
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

	inline auto& getGen() noexcept { static Impl::Gen result; return result; }

	struct InstantEffect
	{
		enum class Type : int
		{
			Add = 0,
			Sub = 1,
			Mul = 2,
			Div = 3
		};

		enum class Stat : int
		{
			SHPS = 0,
			SATK = 1,
			SDEF = 2
		};

		Type type;
		Stat stat;
		float value;

		inline InstantEffect(Type mType, Stat mStat, float mValue) : type{mType}, stat{mStat}, value{mValue} { }
		inline void apply(GameSession& mGameSession, Creature& mX);

		inline std::string getStrType()
		{
			static auto array(ssvu::make_array
			(
				"+",
				"-",
				"*",
				"/"
			));

			return array[static_cast<int>(type)];
		}

		inline std::string getStrStat()
		{
			static auto array(ssvu::make_array
			(
				"HPS",
				"ATK",
				"DEF"
			));

			return array[static_cast<int>(stat)];
		}
	};

	struct Drop
	{
		GameSession& gameSession;
		sf::Sprite card;

		inline Drop(GameSession& mGameSession) : gameSession{mGameSession}
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


	inline auto createElemSprite(int mEI)
	{
		static auto array(ssvu::make_array
		(
			getAssets().eFire,
			getAssets().eWater,
			getAssets().eEarth,
			getAssets().eLightning
		));

		return sf::Sprite{*(array[mEI])};
	}

	template<typename T> inline void appendElems(ssvs::GameWindow& mGW, const T& mX, ElementBitset mEB)
	{
		for(auto i(0u); i < Constants::elementCount; ++i)
		{
			if(!mEB[i]) continue;

			auto offset(7 * i);
			auto s(createElemSprite(i));

			s.setPosition(mX.getPosition() + Vec2f{12.f + offset, 0.f});

			mGW.draw(s);
		}
	}

	inline auto getStatDisplayStr(StatType mBase, StatType mBonus)
	{
		return ssvu::toStr(mBase + mBonus) + " (" + ssvu::toStr(mBase) + "+" + ssvu::toStr(mBonus) + ")";
	}

	struct WeaponStatsDraw
	{
		Vec2f pos;
		sf::Sprite iconATK;
		ssvs::BitmapText txtATK;
		sf::Sprite eST, eWK;

		inline WeaponStatsDraw() : txtATK{*getAssets().fontObStroked, ""}
		{
			iconATK.setTexture(*getAssets().iconATK);
			txtATK.setTracking(-3);
			eST.setTexture(*getAssets().eST);
			eWK.setTexture(*getAssets().eWK);
		}

		inline void commonDraw(Weapon& mW, ssvs::GameWindow& mGW, const Vec2f& mPos, const Vec2f&)
		{
			iconATK.setPosition(mPos + pos);
			eST.setPosition(iconATK.getPosition() + Vec2f{0, 10 + 1});
			eWK.setPosition(eST.getPosition() + Vec2f{0, 6 + 1});
			txtATK.setPosition(iconATK.getPosition() + Vec2f{12.f, 0});

			appendElems(mGW, eST, mW.strongAgainst);
			appendElems(mGW, eWK, mW.weakAgainst);

			mGW.draw(iconATK);
			mGW.draw(txtATK);
			mGW.draw(eST);
			mGW.draw(eWK);
		}

		inline void draw(Weapon& mW, ssvs::GameWindow& mGW, const Vec2f& mPos, const Vec2f& mCenter)
		{
			txtATK.setString(ssvu::toStr(mW.atk));
			commonDraw(mW, mGW, mPos, mCenter);
		}

		inline void draw(Creature& mC, ssvs::GameWindow& mGW, const Vec2f& mPos, const Vec2f& mCenter)
		{
			txtATK.setString(getStatDisplayStr(mC.weapon.atk, mC.bonusATK));
			commonDraw(mC.weapon, mGW, mPos, mCenter);
		}
	};

	struct ArmorStatsDraw
	{
		Vec2f pos;
		sf::Sprite iconDEF;
		ssvs::BitmapText txtDEF;
		sf::Sprite eTY;

		inline ArmorStatsDraw() : txtDEF{*getAssets().fontObStroked, ""}
		{
			iconDEF.setTexture(*getAssets().iconDEF);
			txtDEF.setTracking(-3);
			eTY.setTexture(*getAssets().eTY);
		}

		inline void commonDraw(Armor& mA, ssvs::GameWindow& mGW, const Vec2f& mPos, const Vec2f&)
		{
			iconDEF.setPosition(pos + mPos);
			eTY.setPosition(iconDEF.getPosition() + Vec2f{0, 10 + 1});
			txtDEF.setPosition(iconDEF.getPosition() + Vec2f{12.f, 0});
			mGW.draw(iconDEF);
			mGW.draw(txtDEF);
			mGW.draw(eTY);

			appendElems(mGW, eTY, mA.elementTypes);
		}

		inline void draw(Creature& mC, ssvs::GameWindow& mGW, const Vec2f& mPos, const Vec2f& mCenter)
		{
			txtDEF.setString(getStatDisplayStr(mC.armor.def, mC.bonusDEF));
			commonDraw(mC.armor, mGW, mPos, mCenter);
		}

		inline void draw(Armor& mA, ssvs::GameWindow& mGW, const Vec2f& mPos, const Vec2f& mCenter)
		{
			txtDEF.setString(ssvu::toStr(mA.def));
			commonDraw(mA, mGW, mPos, mCenter);
		}
	};

	struct CreatureStatsDraw
	{
		sf::Sprite iconHPS;
		ssvs::BitmapText txtHPS;

		WeaponStatsDraw wsd;
		ArmorStatsDraw asd;

		inline CreatureStatsDraw() : txtHPS{*getAssets().fontObStroked, ""}
		{
			iconHPS.setTexture(*getAssets().iconHPS);
			txtHPS.setTracking(-3);
		}

		inline void draw(Creature& mC, ssvs::GameWindow& mGW, const Vec2f& mPos, const Vec2f& mCenter)
		{
			txtHPS.setString(ssvu::toStr(mC.hps));
			iconHPS.setPosition(mPos + Vec2f{0.f, 12.f * 0.f});
			txtHPS.setPosition(iconHPS.getPosition() + Vec2f{12.f, 0});

			wsd.pos = Vec2f{0, 12.f};
			wsd.draw(mC, mGW, mPos, mCenter);

			asd.pos = Vec2f{0, wsd.eWK.getPosition().y - mPos.y + 12.f};
			asd.draw(mC, mGW, mPos, mCenter);

			mGW.draw(iconHPS);
			mGW.draw(txtHPS);
		}
	};

	struct WeaponDrop : public Drop
	{
		Weapon weapon;
		WeaponStatsDraw wsd;

		inline WeaponDrop(GameSession& mGameSession) : Drop{mGameSession}
		{
			card.setTexture(*getAssets().equipCard);
		}

		inline void apply(Creature& mX) override
		{
			getAssets().soundPlayer.play(*getAssets().equipWpn);
			mX.weapon = weapon;
		}

		inline void draw(ssvs::GameWindow& mGW, const Vec2f& mPos, const Vec2f& mCenter) override
		{
			Drop::draw(mGW, mPos, mCenter);

			sf::Sprite typeSprite;
			typeSprite.setTexture(*weapon.getTypeTexture());
			typeSprite.setOrigin(Vec2f{typeSprite.getTexture()->getSize()} / 2.f);
			typeSprite.setPosition(card.getPosition());
			mGW.draw(typeSprite);

			wsd.pos = Vec2f{30 - 16, 30 + 6};
			wsd.draw(weapon, mGW, mPos, mCenter);
		}
	};

	struct ArmorDrop : public Drop
	{
		Armor armor;
		ArmorStatsDraw asd;

		inline ArmorDrop(GameSession& mGameSession) : Drop{mGameSession}
		{
			card.setTexture(*getAssets().equipCard);
		}

		inline void apply(Creature& mX) override
		{
			getAssets().soundPlayer.play(*getAssets().equipArmor);
			mX.armor = armor;
		}

		inline void draw(ssvs::GameWindow& mGW, const Vec2f& mPos, const Vec2f& mCenter) override
		{
			Drop::draw(mGW, mPos, mCenter);

			sf::Sprite armorSprite;
			armorSprite.setTexture(*getAssets().armDrop);
			armorSprite.setOrigin(Vec2f{armorSprite.getTexture()->getSize()} / 2.f);
			armorSprite.setPosition(card.getPosition());
			mGW.draw(armorSprite);

			asd.pos = Vec2f{30 - 16, 30 + 6};
			asd.draw(armor, mGW, mPos, mCenter);
		}
	};

	struct DropIE : public Drop
	{
		std::vector<InstantEffect> ies;
		std::vector<ssvs::BitmapText> bts;

		inline DropIE(GameSession& mGameSession) : Drop{mGameSession}
		{

		}

		inline void addIE(InstantEffect mIE)
		{
			ies.emplace_back(mIE);

			ssvs::BitmapText txt{*getAssets().fontObStroked, ""};
			txt.setString(mIE.getStrType() + ssvu::toStr(static_cast<int>(mIE.value)) + " " + mIE.getStrStat());
			txt.setTracking(-3);
			txt.setOrigin(ssvs::getGlobalHalfSize(txt));

			bts.emplace_back(txt);
		}

		inline void apply(Creature& mX) override
		{
			getAssets().soundPlayer.play(*getAssets().powerup, ssvs::SoundPlayer::Mode::Overlap, 1.8f);
			for(auto& x : ies) x.apply(gameSession, mX);
		}

		inline void draw(ssvs::GameWindow& mGW, const Vec2f& mPos, const Vec2f& mCenter) override
		{
			Drop::draw(mGW, mPos, mCenter);

			int i{0};
			for(auto& t : bts)
			{
				t.setPosition(card.getPosition() + Vec2f{0, -15.f + (10 * i)});
				mGW.draw(t);

				++i;
			}
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

		inline std::string getChoiceStr() override { return "Forward"; }
	};


	struct ChoiceCreature : public Choice
	{
		Creature creature;
		CreatureStatsDraw csd;
		sf::Sprite enemySprite;
		float hoverRads;

		inline ChoiceCreature(GameSession& mGameState, SizeT mIdx)
			: Choice{mGameState, mIdx}
		{
			enemySprite.setTexture(*getAssets().enemy);
			enemySprite.setOrigin(Vec2f{enemySprite.getTexture()->getSize()} / 2.f);
			hoverRads = ssvu::getRndR(0.f, ssvu::tau);
		}

		inline void execute() override;
		inline void draw(ssvs::GameWindow&, const Vec2f&, const Vec2f&) override;

		inline std::string getChoiceStr() override { return "Fight"; }
	};

	struct ChoiceItemDrop : public Choice
	{
		sf::Sprite drops;
		ItemDrops itemDrops;

		ChoiceItemDrop(GameSession& mGS, SizeT mIdx);

		inline void execute() override;
		inline void draw(ssvs::GameWindow&, const Vec2f&, const Vec2f&) override;

		inline std::string getChoiceStr() override { return "Collect"; }
	};

	struct ChoiceSingleDrop : public Choice
	{
		ssvu::UPtr<Drop> drop{nullptr};

		ChoiceSingleDrop(GameSession& mGS, SizeT mIdx);

		inline void execute() override;
		inline void draw(ssvs::GameWindow&, const Vec2f&, const Vec2f&) override;

		inline std::string getChoiceStr() override { return "Pickup"; }
	};

	struct GameSession
	{
		enum class State{Playing, Dead, Menu};

		State state{State::Menu};
		int roomNumber{0};
		Creature player;
		ssvu::UPtr<Choice> choices[Constants::maxChoices];
		ssvu::UPtr<Choice> nextChoices[Constants::maxChoices];
		float timer;
		float difficulty{1.f};
		float rndMultiplier{1.2f};

		sf::SoundBuffer* currentMusic{nullptr};
		sf::Sound music;

		ItemDrops* currentDrops{nullptr};

		float shake{0}, deathTextTime{0};
		float difficultyInc{0.03f};

		enum class Mode{Normal, Practice, Hardcore};
		Mode mode{Mode::Normal};
		bool timerEnabled{true};

		inline void sustain()
		{
			if(player.isDead()) return;

			float x(1.f + (roomNumber * 1.5f / difficulty));
			ssvu::clampMax(x, 20);

			eventLo() << "You drain " << static_cast<int>(x) << " HPS defeating the enemy\n";
			player.hps += x;
		}

		inline void restart()
		{
			music.stop();
			getAssets().soundPlayer.stop();

			if(mode == Mode::Normal || mode == Mode::Practice) { difficulty = 1.f; difficultyInc = 0.038f; }
			if(mode == Mode::Hardcore) { difficulty = 1.f; difficultyInc = 0.087f; }

			timerEnabled = (mode != Mode::Practice);

			state = State::Playing;
			roomNumber = 0;
			shake = deathTextTime = 0.f;
			for(auto& c : choices) c.release();
			for(auto& c : nextChoices) c.release();

			Weapon startingWeapon;
			startingWeapon.atk = 5;
			startingWeapon.name = "Starting weapon";
			player.bonusATK = 1;

			Armor startingArmor;
			startingArmor.def = 2;
			startingArmor.name = "Starting armor";
			player.bonusDEF = 1;

			player.name = "Player";
			player.hps = 150;
			player.weapon = startingWeapon;
			player.armor = startingArmor;

			advance();
		}

		inline void gotoMenu()
		{
			music.stop();
			getAssets().soundPlayer.stop();
			shake = deathTextTime = 0.f;

			state = State::Menu;

			currentMusic = getAssets().menu;
			refreshMusic();
		}

		inline GameSession()
		{
			gotoMenu();
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
			if(mode == Mode::Normal || mode == Mode::Practice) timer = ssvu::getSecondsToFT(10);
			else if(mode == Mode::Hardcore) timer = ssvu::getSecondsToFT(6);
		}

		inline void generateRndElements(int mL, ElementBitset& mX)
		{
			auto d(static_cast<int>(mL * difficulty));

			if(roomNumber < 10) return;

			auto i(0u);
			std::vector<int> indices{0, 1, 2, 3};
			std::shuffle(indices.begin(), indices.end(), ssvu::getRndEngine());

			if(ssvu::getRnd(0, 100) < 50) mX[indices[i++]] = true;

			if(d < 20) return;
			if(ssvu::getRnd(0, 100) < 45) mX[indices[i++]] = true;

			if(d < 30) return;
			if(ssvu::getRnd(0, 100) < 40) mX[indices[i++]] = true;

			if(d < 40) return;
			if(ssvu::getRnd(0, 100) < 35) mX[indices[i++]] = true;
		}

		inline int getRndStat(int mL, float, float)
		{
			auto d(static_cast<int>(((mL * 0.8f) + 4) * difficulty));

			return ssvu::getClampedMin(ssvu::getRnd((int)(d * 0.65f), (int)(d * 1.55f)), 0);
//			return ssvu::getClampedMin(1, d + ssvu::getRnd(static_cast<int>((mMultMin * d) * rndMultiplier), static_cast<int>((mMultMax * d) * rndMultiplier)));
		}

		inline InstantEffect generateInstantEffect(InstantEffect::Stat mStat, InstantEffect::Type mType, int mL)
		{
			//auto d(static_cast<int>(mL * difficultyMultiplier));

			// TODO: always 1 positive and negative?
			// TODO: decay every room or low bonuses

	//		InstantEffect::Stat stat;
			//InstantEffect::Type type;
/*
			auto statN = ssvu::getRnd(0, 3);
			if(statN == 0) stat = InstantEffect::Stat::SHPS;
			if(statN == 1) stat = InstantEffect::Stat::SATK;
			if(statN == 2) stat = InstantEffect::Stat::SDEF;
*/
			//auto typeN = ssvu::getRnd(0, 4);
			// auto typeN = ssvu::getRnd(0, 2);
		//	if(typeN == 0) type = InstantEffect::Type::Add;
		//	if(typeN == 1) type = InstantEffect::Type::Sub;
		//	if(typeN == 2) type = InstantEffect::Type::Mul;
		//	if(typeN == 3) type = InstantEffect::Type::Div;

			float val(ssvu::getClampedMin((mL / 8) + ssvu::getRnd(0, 3 + (mL / 12)), 1));
			if(mStat == InstantEffect::Stat::SHPS) val = mL * (10 + ssvu::getRnd(-2, 3));
			// if(typeN == 2 || typeN == 3) val = ssvu::getRndR(0.75f, 1.25f);

			InstantEffect result
			{
				mType, mStat, val
			};

			return result;
		}

		inline auto getShuffledStats()
		{
			std::vector<InstantEffect::Stat> stats
			{
				InstantEffect::Stat::SHPS,
				InstantEffect::Stat::SATK,
				InstantEffect::Stat::SDEF
			};

			std::shuffle(stats.begin(), stats.end(), ssvu::getRndEngine());

			return stats;
		}

		inline auto addIEs(int mL, DropIE& dIE)
		{
			auto ss(getShuffledStats());

			dIE.addIE(generateInstantEffect(ss[0], InstantEffect::Type::Add, mL));
			dIE.addIE(generateInstantEffect(ss[1], InstantEffect::Type::Sub, mL));
		}


		inline auto generateDropIE(int mL)
		{
			auto dIE(ssvu::makeUPtr<DropIE>(*this));

			addIEs(mL, *dIE);

			if(ssvu::getRnd(0, 100) < ssvu::getClampedMax(mL, 35))
			{
				addIEs(mL, *dIE);
			}

			// if(ssvu::getRnd(0, 100) < 25) dIE->addIE(generateInstantEffect(mL));

			return dIE;
		}

		inline auto generateDropWeapon(int mL)
		{
			auto dr(ssvu::makeUPtr<WeaponDrop>(*this));

			dr->weapon = generateWeapon(mL);

			return dr;
		}

		inline auto generateDropArmor(int mL)
		{
			auto dr(ssvu::makeUPtr<ArmorDrop>(*this));

			dr->armor = generateArmor(mL);

			return dr;
		}

		inline ssvu::UPtr<Drop> generateRndDrop(int mL)
		{
			if(ssvu::getRnd(0, 50) > 21)
			{
				return std::move(generateDropIE(mL));
			}
			else
			{
				if(ssvu::getRnd(0, 50) > 19)
					return std::move(generateDropWeapon(mL));
				else
					return std::move(generateDropArmor(mL));
			}
		}

		inline ItemDrops generateDrops(int mL)
		{
//			auto d(static_cast<int>(mL * difficultyMultiplier));

			ItemDrops result;

			auto i(0u);
			result.drops[i] = std::move(generateRndDrop(mL));

			for(; i < Constants::maxDrops; ++i)
			{
				if(ssvu::getRnd(0, 50) > 20) continue;

				result.drops[i] = std::move(generateRndDrop(mL));
			}

			return result;
		}

		inline Weapon generateWeapon(int mL)
		{
			auto d(static_cast<int>(mL * difficulty));

			Weapon result;

			result.name = "Generated name TODO (lvl: " + ssvu::toStr(d) + ")";
			result.atk = getRndStat(mL, 0.5f, 1.8f) + 1;
			generateRndElements(mL, result.strongAgainst);
			generateRndElements(mL, result.weakAgainst);
			result.type = static_cast<Weapon::Type>(ssvu::getRnd(0, 3));

			return result;
		}

		inline Armor generateArmor(int mL)
		{
			auto d(static_cast<int>(mL * difficulty));

			Armor result;

			result.name = "Generated name TODO (lvl: " + ssvu::toStr(d) + ")";
			result.def = getRndStat(mL, 0.5f, 1.8f) * 0.7f;
			generateRndElements(mL, result.elementTypes);

			return result;
		}

		inline Creature generateCreature(int mL)
		{
			auto d(static_cast<int>(mL * difficulty));

			Creature result;

			result.name = getGen().generateCreatureName();
			result.armor = generateArmor(ssvu::getClampedMin(mL * 0.69f + difficulty - 1, 1));
			result.weapon = generateWeapon(mL - 1);
			result.hps = d * 5 + ssvu::getRnd(0, d * 3);

			return result;
		}

		inline ssvu::UPtr<Choice> generateChoiceCreature(int mIdx, int mL)
		{
			auto choice(ssvu::makeUPtr<ChoiceCreature>(*this, mIdx));
			choice->creature = generateCreature((mL + difficulty + (roomNumber / 10)) * difficulty);
			return std::move(choice);
		}

		inline ssvu::UPtr<Choice> generateChoiceSingleDrop(int mIdx, int mL)
		{
			auto choice(ssvu::makeUPtr<ChoiceSingleDrop>(*this, mIdx));
			choice->drop = generateRndDrop(mL);
			return std::move(choice);
		}

		inline ssvu::UPtr<Choice> generateChoiceMultipleDrop(int mIdx, int mL)
		{
			auto choice(ssvu::makeUPtr<ChoiceItemDrop>(*this, mIdx));
			choice->itemDrops = generateDrops(mL);
			return std::move(choice);
		}

		inline void generateChoices()
		{
			auto choiceNumber(2);

			if(roomNumber > 10) choiceNumber = 3;
			else if(roomNumber > 20) choiceNumber = 4;

			std::vector<int> indices{0, 1, 2, 3};
			std::shuffle(indices.begin(), indices.end(), ssvu::getRndEngine());

			for(auto& c : choices) c.release();

			for(int i{0}; i < choiceNumber; ++i)
			{
				auto idx(indices[i]);

				if(ssvu::getRnd(0, 100) > 15)
				{
					choices[idx] = generateChoiceCreature(idx, roomNumber);
				}
				else
				{
					if(ssvu::getRnd(0, 100) > 20)
					{
						choices[idx] = generateChoiceSingleDrop(idx, roomNumber);
					}
					else
					{
						choices[idx] = generateChoiceMultipleDrop(idx, roomNumber);
					}
				}
			}
		}

		inline void refreshMusic()
		{
			if(music.getBuffer() != currentMusic) music.setBuffer(*currentMusic);
			music.setLoop(true);
			if(music.getStatus() != sf::Sound::Status::Playing) music.play();
		}

		template<typename T> inline void resetChoiceAt(SizeT mIdx, T&& mX)
		{
			nextChoices[mIdx] = ssvu::fwd<T>(mX);
		}

		inline void advance()
		{
			++roomNumber;

			if(roomNumber < 10)
			{
				currentMusic = getAssets().lvl1;
				refreshMusic();
			}
			else if(roomNumber < 20)
			{
				currentMusic = getAssets().lvl2;
				refreshMusic();
			}
			else if(roomNumber < 30)
			{
				currentMusic = getAssets().lvl3;
				refreshMusic();
			}
			else if(roomNumber < 40)
			{
				currentMusic = getAssets().lvl4;
				refreshMusic();
			}

			if(roomNumber % 5 == 0)
			{
				eventLo() << "Increasing difficulty...\n";
				difficulty += difficultyInc;
			}

			generateChoices();
			resetTimer();
			endDrops();
		}

		inline void die()
		{
			music.stop();
			getAssets().soundPlayer.play(*getAssets().lose);
			shake = 250;
			deathTextTime = 255;
			state = GameSession::State::Dead;
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
		getAssets().soundPlayer.play(*getAssets().grab);
		gameSession.startDrops(&itemDrops);
		gameSession.resetChoiceAt(idx, ssvu::makeUPtr<ChoiceAdvance>(gameSession, idx));
	}
	inline void ChoiceItemDrop::draw(ssvs::GameWindow& mGW, const Vec2f& mPos, const Vec2f&)
	{
		drops.setPosition(mPos);
		mGW.draw(drops);
	}

	inline ChoiceSingleDrop::ChoiceSingleDrop(GameSession& mGS, SizeT mIdx) : Choice{mGS, mIdx}
	{

	}
	inline void ChoiceSingleDrop::execute()
	{
		if(drop == nullptr) return;

		drop->apply(gameSession.player);
		gameSession.resetChoiceAt(idx, ssvu::makeUPtr<ChoiceAdvance>(gameSession, idx));
	}
	inline void ChoiceSingleDrop::draw(ssvs::GameWindow& mGW, const Vec2f& mPos, const Vec2f& mCenter)
	{
		if(drop == nullptr) return;

		drop->draw(mGW, mPos, mCenter);
	}


	inline void ChoiceCreature::execute()
	{
		gameSession.player.weapon.playAttackSounds();

		if(gameSession.player.canDamage(creature))
		{
			gameSession.player.fight(creature);

			gameSession.sustain();

			getAssets().soundPlayer.play(*getAssets().drop);
			gameSession.resetChoiceAt(idx, ssvu::makeUPtr<ChoiceItemDrop>(gameSession, idx));

			gameSession.shake = 10;
		}
		else
		{
			eventLo() << gameSession.player.name << " cannot fight " << creature.name << "!\n";
		}
	}
	inline void ChoiceCreature::draw(ssvs::GameWindow& mGW, const Vec2f& mPos, const Vec2f& mCenter)
	{
		Vec2f offset{4.f, 4.f};
		hoverRads = ssvu::wrapRad(hoverRads + 0.05f);
		enemySprite.setPosition(mCenter + Vec2f(0, std::sin(hoverRads) * 4.f));
		mGW.draw(enemySprite);
		csd.draw(creature, mGW, offset + mPos, mCenter);
	}

	inline void InstantEffect::apply(GameSession& mGameSession, Creature& mX)
	{
		StatType* statPtr{nullptr};

		switch(stat)
		{
			case Stat::SHPS: statPtr = &mX.hps; break;
			case Stat::SATK: statPtr = &mX.bonusATK; break;
			case Stat::SDEF: statPtr = &mX.bonusDEF; break;
		}

		float x(static_cast<float>(*statPtr));

		switch(type)
		{
			case Type::Add: *statPtr += value; break;
			case Type::Sub: *statPtr -= value; break;
			case Type::Mul: *statPtr = static_cast<int>(x * value); break;
			case Type::Div: *statPtr = static_cast<int>(x / value); break;
		}

		eventLo() << "Got " << getStrType() << ssvu::toStr(static_cast<int>(value)) << " " << getStrStat() << "!\n";

		mX.checkBurns(mGameSession);
	}

	inline void Creature::checkBurns(GameSession& mGameSession)
	{
		int burn{0};

		if(bonusATK < 0)
		{
			burn -= bonusATK;
			bonusATK = 0;
		}

		if(bonusDEF < 0)
		{
			burn -= bonusDEF;
			bonusDEF = 0;
		}

		if(burn == 0) return;

		auto x(burn * (5 * mGameSession.roomNumber * mGameSession.difficulty));

		hps -= x;
		eventLo() << name << " suffers " << x << " stat burn dmg!\n";
	}

	struct SlotChoice
	{
		sf::RectangleShape shape;
		sf::Sprite sprite;
		ssvs::BitmapText txtNum;
		ssvs::BitmapText txtStr;
		int choice;

		static constexpr float step{300.f / 4.f};

		inline SlotChoice(int mChoice) : txtNum{*getAssets().fontObBig, ssvu::toStr(mChoice + 1)},
			txtStr{*getAssets().fontObStroked, ""}, choice{mChoice}
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
			Vec2f oldPos;
			ssvs::BitmapText txtDeath, txtRestart, txtCredits;

			inline void initInput()
			{
				auto& gs(gameState);

				gs.addInput({{IK::Escape}}, [this](FT){ if(gameSession.state != GameSession::State::Menu) gameSession.gotoMenu(); }, IT::Once);

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
				if(gameSession.state == GameSession::State::Menu)
				{
					if(mI == 0)
					{
						gameSession.mode = GameSession::Mode::Normal;
						gameSession.restart();
					}

					if(mI == 1)
					{
						gameSession.mode = GameSession::Mode::Practice;
						gameSession.restart();
					}

					if(mI == 2)
					{
						gameSession.mode = GameSession::Mode::Hardcore;
						gameSession.restart();
					}

					if(mI == 3)
					{
						gameWindow->stop();
					}

					return;
				}

				if(gameSession.state == GameSession::State::Dead)
				{
					if(mI == 0) gameSession.gotoMenu();
					else gameSession.restart();

					return;
				}

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

				if(gameSession.deathTextTime > 0) gameSession.deathTextTime -= mFT;

				if(gameSession.state == GameSession::State::Playing)
				{
					if(gameSession.timerEnabled) gameSession.timer -= mFT;

					if(!gameSession.player.isDead())
					{
						if(gameSession.timer <= ssvu::getSecondsToFT(1))
						{
							if(gameSession.shake < 3) gameSession.shake = 3;
						}
						else if(gameSession.timer <= ssvu::getSecondsToFT(2))
						{
							if(gameSession.shake < 2) gameSession.shake = 2;
						}
						else if(gameSession.timer <= ssvu::getSecondsToFT(3))
						{
							if(gameSession.shake < 1) gameSession.shake = 1;
						}
					}

					if(gameSession.timer <= 0 || gameSession.player.isDead())
					{
						gameSession.die();
					}
					else
					{
						auto intt(ssvu::getFTToSeconds(static_cast<int>(gameSession.timer)));
						auto gts(intt >= 10 ? ssvu::toStr(intt) : "0" + ssvu::toStr(intt));

						auto third(gameWindow->getWidth() / 5.f);

						if(gameSession.timerEnabled)
						{
							txtTimer.setString("00:" + gts);
						}
						else
						{
							txtTimer.setString("PRACTICE");
						}

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

						if(!els.empty())
						{
							std::string elsLog;

							int foundNewLines{0};

							for(auto itr(els.rbegin()); itr < els.rend(); ++itr)
							{
								if(*itr == '\n') ++foundNewLines;
								if(foundNewLines == 6) break;
								elsLog += *itr;
							}

							std::string final{elsLog.rbegin(), elsLog.rend()};
							tempLog.setString(final);
						}
					}
				}
				else if(gameSession.state == GameSession::State::Menu)
				{

				}

				if(gameSession.shake > 0)
				{
					gameSession.shake -= mFT;
					auto shake(gameSession.shake);
					gameCamera.setCenter(oldPos + Vec2f{ssvu::getRndR(-shake, shake), ssvu::getRndR(-shake, shake)});
				}
				else
				{
					gameCamera.setCenter(oldPos);
				}
			}

			inline void drawPlaying()
			{
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

			inline void draw()
			{
				gameCamera.apply();


				if(gameSession.state == GameSession::State::Playing || gameSession.deathTextTime > 0)
				{
					drawPlaying();
				}


				gameCamera.unapply();

				txtDeath.setOrigin(ssvs::getGlobalHalfSize(txtDeath));
				txtRestart.setOrigin(ssvs::getGlobalHalfSize(txtRestart));
				txtCredits.setOrigin(Vec2f{ssvs::getLocalLeft(txtCredits), ssvs::getLocalBottom(txtCredits)});


				txtCredits.setPosition(5, 240 - 5);

				if(gameSession.state == GameSession::State::Dead)
				{
					txtDeath.setString("You have perished.");
					txtRestart.setString("Press 1 for menu.\nPress any number to restart.\n\nYou reached room " + ssvu::toStr(gameSession.roomNumber) + ".");

					txtDeath.setPosition(320 / 2.f, 80);
					txtRestart.setPosition(320 / 2.f, 120);

					gameWindow->draw(txtDeath);
					gameWindow->draw(txtRestart);

					txtDeath.setColor(sf::Color(255, 255, 255, 255 - static_cast<unsigned char>(gameSession.deathTextTime)));
					txtRestart.setColor(sf::Color(255, 255, 255, 255 - static_cast<unsigned char>(gameSession.deathTextTime)));
				}

				if(gameSession.state == GameSession::State::Menu)
				{
					txtDeath.setString("DELVER'S CHOICE");
					txtDeath.setColor(sf::Color(255, 255, 255, 255));

					txtDeath.setPosition(320 / 2.f, 30);
					txtRestart.setPosition(320 / 2.f, 70);

					txtRestart.setString("1. Normal mode\n2. Pratice mode\n3. Hardcore mode\n4. Exit");
					txtRestart.setColor(sf::Color(255, 255, 255, 255));

					gameWindow->draw(txtDeath);
					gameWindow->draw(txtRestart);
					gameWindow->draw(txtCredits);
				}
			}

		public:
			inline GameApp(ssvs::GameWindow& mGameWindow)
				: Boilerplate::App{mGameWindow}, tempLog{*getAssets().fontObStroked, ""}, txtTimer{*getAssets().fontObBig, ""},
				  txtRoom{*getAssets().fontObBig, ""}, txtDeath{*getAssets().fontObBig, "You have perished."}, txtRestart{*getAssets().fontObStroked, ""},
				   txtCredits{*getAssets().fontObStroked, ""}
			{
				tempLog.setTracking(-3);
				txtTimer.setTracking(-1);
				txtRoom.setTracking(-1);
				txtDeath.setTracking(-1);
				txtRestart.setTracking(-3);
				txtCredits.setTracking(-3);

				txtCredits.setString("Global Game Jam 2015\n"
									 "Developer: Vittorio Romeo\n"
									 "2D Artist: Vittorio Romeo\n"
									 "Audio: Nicola Bombaci\n"
									 "Designer: Sergio Zavettieri\n"
									 "Additional help: Davide Iuffrida\n\n"
									 "http://vittorioromeo.info\n"
									 "http://nicolabombaci.com");

				for(int i{0}; i < 4; ++i)
					slotChoices.emplace_back(i);

				gameState.onUpdate += [this](FT mFT){ update(mFT); };
				gameState.onDraw += [this]{ draw(); };

				dropsModalSprite.setTexture(*getAssets().dropsModal);
				dropsModalSprite.setPosition(10, 40);

				tempLog.setPosition(Vec2f{75, 180});



				initInput();

				oldPos = gameCamera.getCenter();

				gameSession.gotoMenu();
			}
	};
}

int main()
{
	Boilerplate::AppRunner<ggj::GameApp>{"Delver's choice - GGJ2015 - RC6", 320, 240};
	return 0;
}


