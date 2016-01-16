#include "LevelEditorState.h"

#define MENU_WIDTH 150

constexpr auto txtSwitchMode = "Switch mode";
constexpr auto txtLoadDefault = "Load Default";
constexpr auto txtLoad = "Load";
constexpr auto txtSave = "Save";
constexpr auto txtQuit = "Quit";

LevelEditorState::LevelEditorState(const int w, const int h, std::shared_ptr<Player> player, std::shared_ptr<LevelReaderWriter> levelReader) : m_windowWidth(w), m_windowHeight(h),
m_player(move(player)),
m_levelReader(move(levelReader))
{
	m_scale = float(h) / m_levelReader->getLevel().size();

	sf::Color textColor = sf::Color::Yellow;
	
	//FIXME create separate font loader
	m_font.loadFromFile("resources/font/OtherF.ttf");

	m_statusBar.setFont(m_font);
	m_statusBar.setString("Wall Edit Mode (Hit Space to switch)");
	m_statusBar.setCharacterSize(30);
	
	m_statusBar.setPosition(m_windowWidth / 2.0f, 1.0f);
	m_statusBar.setOrigin(m_statusBar.getGlobalBounds().width / 2.0f, m_statusBar.getGlobalBounds().height / 2.0f);

	m_statusBar.setColor(textColor);

	m_customLevels = m_levelReader->getCustomLevels();

	//Gui
	m_gui = std::make_unique<LevelEditorGui>(w - MENU_WIDTH + 1, 10, MENU_WIDTH);
	m_gui->addButton(txtSwitchMode);
	m_gui->addButton(txtLoadDefault);
	
	for (auto& cl : m_customLevels)
	{
		m_gui->addButton(cl);
	}

	m_gui->addButton(txtSave);
	m_gui->addButton(txtQuit);
	
}


LevelEditorState::~LevelEditorState()
{
	//Empty
}

void LevelEditorState::update(const float ft)
{
	//Empty
}

void LevelEditorState::draw(sf::RenderWindow & window)
{
	
	window.clear();
	
	const size_t levelSize = m_levelReader->getLevel().size();

	//Render map background
	sf::RectangleShape minimapBg(sf::Vector2f(m_windowWidth, m_windowHeight));
	minimapBg.setPosition(0, 0);
	minimapBg.setFillColor(sf::Color(150, 150, 150, 255));
	window.draw(minimapBg);

	// Render Player on map
	{
		sf::CircleShape player(minimapScale, 3);
		player.setPosition(m_player->m_posY * m_scale, m_player->m_posX * m_scale);
		player.setFillColor(sf::Color(255, 255, 255, 255));
		player.setOrigin(minimapScale, minimapScale);

		sf::RectangleShape player2(sf::Vector2f(minimapScale / 2.0f, minimapScale / 2.0f));
		player2.setPosition(m_player->m_posY * m_scale, m_player->m_posX * m_scale);
		player2.setFillColor(sf::Color(255, 255, 255, 255));
		player2.setOrigin(minimapScale / 4.0f, -minimapScale / 2.0f);

		float angle = std::atan2f(m_player->m_dirX, m_player->m_dirY);
		player.setRotation((angle * 57.2957795f) + 90);
		player2.setRotation((angle * 57.2957795f) + 90);

		window.draw(player);
		window.draw(player2);
	}

	//walls
	for (size_t y = 0; y < levelSize; y++)
	{
		for (size_t x = 0; x < levelSize; x++)
		{
			int id = m_levelReader->getLevel()[y][x];
			if (id > 0 && id < 9)
			{
				sf::RectangleShape wall(sf::Vector2f(m_scale - 2.0f, m_scale - 2.0f));
				wall.setPosition(x * m_scale, y * m_scale);
				wall.setTexture(m_levelReader->getTextureSfml(id - 1));
				wall.setOutlineThickness(2);
				wall.setOutlineColor(sf::Color(0, 0, 0));
				window.draw(wall);
			}
		}
	}
	// Render entities on minimap
	const size_t spritesSize = m_levelReader->getSprites().size();
	for (int i = 0; i < spritesSize; i++)
	{
		sf::RectangleShape object(sf::Vector2f(m_scale - 2.0f, m_scale - 2.0f));
		object.setPosition(m_levelReader->getSprites()[i].y * m_scale, m_levelReader->getSprites()[i].x * m_scale);
		object.setOrigin(m_scale / 2.0f, m_scale / 2.0f);
		object.setOutlineThickness(2);
		
		if (i == m_entitySelected)
		{
			object.setOutlineColor(sf::Color(0, 255, 0, 255));
		}
		else
		{
			object.setOutlineColor(sf::Color(255, 255, 255, 128));
		}
				
		object.setTexture(m_levelReader->getTextureSfml(m_levelReader->getSprites()[i].texture));
		window.draw(object);
	}

	if (m_editEntities && (m_entitySelected != -1))
	{
		sf::RectangleShape mouseRect(sf::Vector2f(m_scale - 1.0f, m_scale - 1.0f));
		mouseRect.setPosition(m_mousePos.x, m_mousePos.y);
		mouseRect.setOrigin(m_scale / 2.0f, m_scale / 2.0f);
		mouseRect.setOutlineThickness(1);
		mouseRect.setOutlineColor(sf::Color(255, 255, 255, 80));
		mouseRect.setFillColor(sf::Color(0, 0, 0, 0));
		window.draw(mouseRect);
	}

	window.draw(m_statusBar);

	m_gui->draw(window);

	window.display();

}

void LevelEditorState::handleInput(const sf::Event & event, const sf::Vector2f & mousepPosition, Game & game)
{
	
	m_mousePos = mousepPosition;
	
	//gui events
	m_gui->handleInput(event, mousepPosition);

	//process gui events first
	if (m_gui->getPressed(txtSwitchMode))
	{
		toggleMode();
	}
	if (m_gui->getPressed(txtLoadDefault))
	{
		//reset player position
		m_player->m_posX = 22.0;
		m_player->m_posY = 11.5;
		m_player->m_dirX = -1.0;
		m_player->m_dirY = 0.0;
		m_player->m_planeX = 0.0;
		m_player->m_planeY = 0.66;

		//reload level
		m_levelReader->reloadLevel();
	}
	for (auto& cl : m_customLevels)
	{
		if (m_gui->getPressed(cl))
		{
			m_levelReader->loadCustomLevel(cl);
		}
	}
	if (m_gui->getPressed(txtSave))
	{
		//TODO: handle save
	}
	if (m_gui->getPressed(txtQuit))
	{
		game.changeState(Game::GameStateName::MAINMENU);
	}



	//process button press
	if (event.type == sf::Event::KeyReleased)
	{
		if (event.key.code == sf::Keyboard::Escape)
		{
			game.changeState(Game::GameStateName::MAINMENU);
		}
		//entity create and delete
		if (m_editEntities)
		{
			if (event.key.code == sf::Keyboard::Q) //barrel
			{
				m_levelReader->createSprite(mousepPosition.y / m_scale, mousepPosition.x / m_scale, 10);
			}
			if (event.key.code == sf::Keyboard::W) //pillar
			{
				m_levelReader->createSprite(mousepPosition.y / m_scale, mousepPosition.x / m_scale, 11);
			}
			if (event.key.code == sf::Keyboard::E) //light
			{
				m_levelReader->createSprite(mousepPosition.y / m_scale, mousepPosition.x / m_scale, 12);
			}
			if (m_entitySelected != -1)
			{
				if (event.key.code == sf::Keyboard::Left) 
				{
					Sprite spr = m_levelReader->getSprites()[m_entitySelected];
					m_levelReader->moveSprite(m_entitySelected, spr.x, spr.y -= 0.1);
				}
				if (event.key.code == sf::Keyboard::Right)
				{
					Sprite spr = m_levelReader->getSprites()[m_entitySelected];
					m_levelReader->moveSprite(m_entitySelected, spr.x, spr.y += 0.1);
				}
				if (event.key.code == sf::Keyboard::Up)
				{
					Sprite spr = m_levelReader->getSprites()[m_entitySelected];
					m_levelReader->moveSprite(m_entitySelected, spr.x -= 0.1, spr.y);
				}
				if (event.key.code == sf::Keyboard::Down)
				{
					Sprite spr = m_levelReader->getSprites()[m_entitySelected];
					m_levelReader->moveSprite(m_entitySelected, spr.x += 0.1, spr.y);
				}				
				if (event.key.code == sf::Keyboard::Delete)
				{
					m_levelReader->deleteSprite(m_entitySelected);
					m_entitySelected = -1;
				}
			}
		}
		if (event.key.code == sf::Keyboard::Space)
		{
			toggleMode();
		}
	}
	
	//ignore mouse events in menu
	const size_t levelSize = m_levelReader->getLevel().size();
	if (mousepPosition.x > levelSize * m_scale) {
		return;
	}
	
	//process mouse click
	if (event.type == sf::Event::MouseButtonPressed)
	{
		if (m_editEntities)
		{
			
			if (m_entitySelected != -1)
			{
				//move sprite data
				m_levelReader->moveSprite(m_entitySelected, mousepPosition.y / m_scale, mousepPosition.x / m_scale);
				m_entitySelected = -1;
			}
			else
			{
				const size_t spritesSize = m_levelReader->getSprites().size();
				for (int i = 0; i < spritesSize; i++)
				{
					sf::RectangleShape object(sf::Vector2f(m_scale - 2.0f, m_scale - 2.0f));
					object.setPosition(m_levelReader->getSprites()[i].y * m_scale, m_levelReader->getSprites()[i].x * m_scale);
					object.setOrigin(m_scale / 2.0f, m_scale / 2.0f);
					if (object.getGlobalBounds().contains(mousepPosition))
					{
						m_entitySelected = i;
					}
				}
			}
			
			return;
		}

		const int x = mousepPosition.x / m_scale;
		const int y = mousepPosition.y / m_scale;

		const int value = m_levelReader->getLevel()[y][x];

		// 1 ... 8 - walls
		// 9, 10 - floor, ceiling
		// 11,12,13 - barrel, pillar, light
		// In fact you have to substract 1 for the level 
		if (value < 8 && (event.mouseButton.button == sf::Mouse::Left))
		{
			m_levelReader->changeLevelTile(y, x, value + 1);
		}
		else
		{
			const int levelSize = m_levelReader->getLevel().size();
			//Do not allow to delete the level outer walls, this breaks the raycaster
			if (x == 0 || x == levelSize-1) 
			{
				m_levelReader->changeLevelTile(y, x, 1);
			}
			else if (y == 0 || y == levelSize-1) 
			{
				m_levelReader->changeLevelTile(y, x, 1);
			} 
			else
			{
				m_levelReader->changeLevelTile(y, x, 0);
			}
		}
	}
}

void LevelEditorState::toggleMode()
{
	m_editEntities = !m_editEntities;

	if (!m_editEntities)
	{
		m_statusBar.setString("Wall Edit Mode (Hit Space to switch)");
		m_statusBar.setOrigin(m_statusBar.getGlobalBounds().width / 2.0f, m_statusBar.getGlobalBounds().height / 2.0f);
	}
	else
	{
		m_statusBar.setString("Entities Edit Mode (Hit Space to switch)");
		m_statusBar.setOrigin(m_statusBar.getGlobalBounds().width / 2.0f, m_statusBar.getGlobalBounds().height / 2.0f);
	}
}


