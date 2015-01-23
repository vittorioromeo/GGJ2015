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

			inline Assets()
			{
				ssvs::loadAssetsFromJson(assetManager, "Data/", ssvj::Val::fromFile("Data/assets.json"));

				obStroked = &assetManager.get<ssvs::BitmapFont>("fontObStroked");
				obBig = &assetManager.get<ssvs::BitmapFont>("fontObBig");
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

	struct EventLog
	{
		template<typename T> inline auto operator<<(const T& mX)
		{
			getEventLogStream() << mX;
			ssvu::lo() << mX;
			return EventLog{};
		}
	};

	inline auto eventLog() noexcept
	{
		return EventLog{};
	}

	using HPS = int;
	using ATK = int;
	using DEF = int;

	struct Constants
	{
		static constexpr SizeT elementCount{4};
		static constexpr float bonusMultiplier{1.5f};
		static constexpr float malusMultiplier{0.8f};
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

		inline static auto getWeaponDamageAgainst(const Weapon& mW, const Armor& mA)
		{
			auto result(mW.atk - mA.def);
			if(isWeaponStrongAgainst(mW, mA)) result *= Constants::bonusMultiplier;
			if(isWeaponWeakAgainst(mW, mA)) result *= Constants::malusMultiplier;
			return ssvu::getClampedMin(result, 0);
		}

		inline static bool canWeaponDamage(const Weapon& mW, const Armor& mA)
		{
			return getWeaponDamageAgainst(mW, mA) > 0;
		}
	};

	struct Creature
	{
		std::string name{"Unnamed"};
		Weapon weapon;
		Armor armor;
		HPS hps{-1};

		inline void attackOnce(Creature& mX)
		{
			auto dmg(Calculations::getWeaponDamageAgainst(weapon, mX.armor));
			mX.hps -= dmg;

			eventLog() << name << " attacks " << mX.name << " for " << dmg << " points of damage!\n";

			if(Calculations::isWeaponStrongAgainst(weapon, mX.armor))
				eventLog() << "(Strong attack!)\n";

			if(Calculations::isWeaponWeakAgainst(weapon, mX.armor))
				eventLog() << "(Weak attack!)\n";

			if(mX.isDead())
				eventLog() << mX.name << " is dead!\n";

			eventLog() << "\n";
		}

		inline void fight(Creature& mX)
		{
			eventLog() << name << " starts fighting " << mX.name << "!\n";

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
			return Calculations::canWeaponDamage(weapon, mX.armor);
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

	class GameSession;

	struct Choice
	{
		GameSession& gameSession;
		SizeT idx;

		inline Choice(GameSession& mGameState, SizeT mIdx) : gameSession{mGameState}, idx{mIdx} { }

		inline virtual void execute() { }

		inline virtual std::string getChoiceStr() { return ""; }
	};

	struct ChoiceAdvance : public Choice
	{
		inline ChoiceAdvance(GameSession& mGameState, SizeT mIdx) : Choice{mGameState, mIdx} { }

		inline void execute() override;
		inline std::string getChoiceStr() override
		{
			return "Advance to next room";
		}
	};

	struct ChoiceCreature : public Choice
	{
		Creature creature;

		inline ChoiceCreature(GameSession& mGameState, SizeT mIdx) : Choice{mGameState, mIdx} { }

		inline void execute() override;
		inline std::string getChoiceStr() override
		{
			return "Fight " + creature.name + "\n(" + creature.getLogStr() + ")";
		}
	};

	struct ChoiceItemDrop : public Choice
	{
		inline ChoiceItemDrop(GameSession& mGameState, SizeT mIdx) : Choice{mGameState, mIdx} { }

		inline void execute() override;
		inline std::string getChoiceStr() override
		{
			return "Collect dropped items";
		}
	};

	struct GameSession
	{
		int roomNumber{0};
		Creature player;
		std::vector<ssvu::UPtr<Choice>> choices;
		float timer;
		ssvu::Delegate<void()> onRefreshChoices;

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

		inline void refreshChoices()
		{
			onRefreshChoices();
			onRefreshChoices = ssvu::Delegate<void()>{};
		}

		inline void resetTimer()
		{
			timer = ssvu::getSecondsToFT(10);
		}

		inline Weapon generateWeapon(int mL)
		{
			Weapon result;

			result.name = "Generated name TODO (lvl: " + ssvu::toStr(mL) + ")";
			result.atk = ssvu::getClampedMin(5, mL + ssvu::getRnd(static_cast<int>(-1.5f * mL), static_cast<int>(1.8f * mL)));
			for(int i{1}; i < mL / 3; ++i) if(ssvu::getRnd(0, 100) < 25) result.strongAgainst[ssvu::getRnd(0ul, Constants::elementCount)] = true;
			for(int i{1}; i < mL / 3; ++i) if(ssvu::getRnd(0, 100) < 25) result.weakAgainst[ssvu::getRnd(0ul, Constants::elementCount)] = true;

			return result;
		}

		inline Armor generateArmor(int mL)
		{
			Armor result;

			result.name = "Generated name TODO (lvl: " + ssvu::toStr(mL) + ")";
			result.def = ssvu::getClampedMin(5, mL + ssvu::getRnd(static_cast<int>(-1.5f * mL), static_cast<int>(1.8f * mL))) * 0.7f;
			for(int i{1}; i < mL / 3; ++i) if(ssvu::getRnd(0, 100) < 25) result.elementTypes[ssvu::getRnd(0ul, Constants::elementCount)] = true;

			return result;
		}

		inline Creature generateCreature(int mL)
		{
			Creature result;

			result.name = "Generated name TODO (lvl: " + ssvu::toStr(mL) + ")";
			result.armor = generateArmor(mL);
			result.weapon = generateWeapon(mL);
			result.hps = mL * 5 + ssvu::getRnd(0, mL * 3);

			return result;
		}

		inline void generateChoices()
		{
			choices.clear();

			for(int i{0}; i < 5; ++i)
			{
				auto choice(ssvu::makeUPtr<ChoiceCreature>(*this, i));
				choice->creature = generateCreature(roomNumber);

				choices.emplace_back(std::move(choice));
			}
		}


		template<typename T> inline void resetChoiceAt(SizeT mIdx, T&& mX)
		{
			//onRefreshChoices += [this, mIdx, &mX]{
				choices[mIdx] = ssvu::fwd<T>(mX);// };
		}

		inline void advance()
		{
			++roomNumber;
			generateChoices();
			resetTimer();
		}
	};

	inline void ChoiceAdvance::execute()
	{
		gameSession.advance();
	}

	inline void ChoiceItemDrop::execute()
	{
		gameSession.resetChoiceAt(idx, ssvu::makeUPtr<ChoiceAdvance>(gameSession, idx));
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
			eventLog() << gameSession.player.name << " cannot fight " << creature.name << "!\n";
		}
	}

	class GameApp : public Boilerplate::App
	{
		private:
			GameSession gameSession;
			ssvs::BitmapText tempLog;
			ssvs::BitmapText txtTimer;

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

				gs.addInput({{IK::Num0}}, [this](FT){ executeChoice(0); }, IT::Once);
				gs.addInput({{IK::Num1}}, [this](FT){ executeChoice(1); }, IT::Once);
				gs.addInput({{IK::Num2}}, [this](FT){ executeChoice(2); }, IT::Once);
				gs.addInput({{IK::Num3}}, [this](FT){ executeChoice(3); }, IT::Once);
				gs.addInput({{IK::Num4}}, [this](FT){ executeChoice(4); }, IT::Once);
				gs.addInput({{IK::Num5}}, [this](FT){ executeChoice(5); }, IT::Once);
				gs.addInput({{IK::Num6}}, [this](FT){ executeChoice(6); }, IT::Once);
				gs.addInput({{IK::Num7}}, [this](FT){ executeChoice(7); }, IT::Once);
				gs.addInput({{IK::Num8}}, [this](FT){ executeChoice(8); }, IT::Once);
				gs.addInput({{IK::Num9}}, [this](FT){ executeChoice(9); }, IT::Once);
			}

			inline void executeChoice(int mI)
			{
				if(gameSession.choices.size() >= mI) return;

				gameSession.refreshChoices();
				gameSession.choices[mI]->execute();
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

					txtTimer.setString("00:" + gts);
					txtTimer.setOrigin(ssvs::getGlobalHalfSize(txtTimer));
					txtTimer.setPosition(gameWindow->getWidth() / 2.f, 25);

					std::string choiceStr;

					int i{0};
					for(auto& c : gameSession.choices)
					{
						choiceStr += ssvu::toStr(i) + ". " + c->getChoiceStr() + "\n\n";
						++i;
					}

					auto els(getEventLogStream().str());
					els = els.substr(els.size() > 250 ? els.size() - 250 : 0, 250);
					//getEventLogStream().str("");

					tempLog.setString("\n\n" + choiceStr + "\n\n" + els);
				}


			}

			inline void draw()
			{
				gameCamera.apply();
				{
					gameWindow->draw(tempLog);
				}
				gameCamera.unapply();

				gameWindow->draw(txtTimer);
			}

		public:
			inline GameApp(ssvs::GameWindow& mGameWindow)
				: Boilerplate::App{mGameWindow}, tempLog{*getAssets().obStroked, ""}, txtTimer{*getAssets().obBig, ""}
			{
				tempLog.setTracking(-3);
				txtTimer.setTracking(-1);



				gameState.onUpdate += [this](FT mFT){ update(mFT); };
				gameState.onDraw += [this]{ draw(); };

				initInput();
				//initTest();
			}
	};
}

int main()
{
	Boilerplate::AppRunner<ggj::GameApp>{"GGJ2015", 800, 600};
	return 0;
}

