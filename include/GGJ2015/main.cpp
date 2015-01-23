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

			inline Assets()
			{
				ssvs::loadAssetsFromJson(assetManager, "Data/", ssvj::Val::fromFile("Data/assets.json"));

				obStroked = &assetManager.get<ssvs::BitmapFont>("fontObStroked");
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

					SSVU_ASSERT(elementTypes.size() == Constants::elementsCount);
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

		inline bool isDead() const noexcept { return hps <= 0; }
	};

	class GameSession;

	struct Choice
	{
		GameSession& gameState;

		inline Choice(GameSession& mGameState) : gameState{mGameState}
		{

		}

		inline virtual void execute()
		{

		}
	};

	struct ChoiceCreature : public Choice
	{
		Creature creature;
		inline void execute() override;
	};

	struct GameSession
	{
		Creature player;

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
		}
	};

	inline void ChoiceCreature::execute()
	{
		gameState.player.fight(creature);
	}

	class GameApp : public Boilerplate::App
	{
		private:
			GameSession gameSession;
			ssvs::BitmapText tempLog;

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
			}

			inline void update(FT mFT)
			{
				gameCamera.update<float>(mFT);

				tempLog.setString(getEventLogStream().str());
			}

			inline void draw()
			{
				gameCamera.apply();
				{
					gameWindow->draw(tempLog);
				}
				gameCamera.unapply();
			}

		public:
			inline GameApp(ssvs::GameWindow& mGameWindow)
				: Boilerplate::App{mGameWindow}, tempLog{*getAssets().obStroked, ""}
			{
				tempLog.setTracking(-3);

				gameState.onUpdate += [this](FT mFT){ update(mFT); };
				gameState.onDraw += [this]{ draw(); };

				initInput();
				//initTest();
			}
	};
}

int main()
{
	using namespace ggj;

	Weapon w1;
	w1.name = "Test W1";
	w1.atk = 15;
	w1.strongAgainst = ElementBitset{"1000"};

	Weapon w2;
	w2.name = "Test W2";
	w2.atk = 12;

	Armor a1;
	a1.name = "Test A1";
	a1.def = 2;
	a1.elementTypes = ElementBitset{"0101"};

	Armor a2;
	a2.name = "Test A2";
	a2.def = 4;
	a2.elementTypes = ElementBitset{"1101"};

	Creature c1;
	c1.name = "Test C1";
	c1.hps = 100;
	c1.weapon = w1;
	c1.armor = a1;

	Creature c2;
	c2.name = "Test C2";
	c2.hps = 90;
	c2.weapon = w2;
	c2.armor = a2;

	c1.fight(c2);

	Boilerplate::AppRunner<ggj::GameApp>{"GGJ2015", 800, 600};
	return 0;
}

