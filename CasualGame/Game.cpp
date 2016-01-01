#include "Game.h"

Game::Game()
{
	
	m_window = std::make_unique<sf::RenderWindow>(sf::VideoMode(GameConfig::windowWidth, GameConfig::windowHeight), GameConfig::gameTitle);
	m_clock = std::make_unique<sf::Clock>();
	m_currentState = std::make_unique<MainMenuState>();

	m_window->setFramerateLimit(500);
	m_window->setVerticalSyncEnabled(true);
}

void Game::run()
{
	while (m_running &&  m_window->isOpen())
	{
		checkInput();
		update();
		draw();
		updateTimers();
	}

	m_window->close();

}

void Game::checkInput()
{
	sf::Event event;
	while (m_window->pollEvent(event))
	{
		if (event.type == sf::Event::Closed)
		{
			m_running = false;
		} 
		else
		{
			sf::Vector2f mousePosition = (sf::Vector2f) sf::Mouse::getPosition(*m_window);
			m_currentState->handleInput(event, mousePosition, *this);
		}
	}
}

void Game::update() 
{
	m_currentSlice += m_lastFt;

	for (; m_currentSlice >= GameConfig::ftSlice; m_currentSlice -= GameConfig::ftSlice)
	{
		m_currentState->update(GameConfig::ftStep);
	}
}

void Game::draw()
{
	m_currentState->draw(*m_window);
}

void Game::updateTimers()
{
	sf::Int32 elapsedTime = m_clock->restart().asMilliseconds();
	float ft = static_cast<float>(elapsedTime);
	m_lastFt = ft;
	
	if (m_fpsShowTimer == 0)
	{
		m_fpsShowTimer = 5;
		float ftSeconds = ft / 1000.f;
		if (ftSeconds > 0.f)
		{
			int fps = static_cast<int>(1.f / ftSeconds);
			m_window->setTitle("FPS: " + std::to_string(fps) + " " + GameConfig::gameTitle);
		}
	}
	m_fpsShowTimer--;
}

void Game::changeState(GameStateName newState)
{
	switch (newState)
	{
	case GameStateName::MAINMENU:
		m_currentState = std::move(std::unique_ptr<MainMenuState>(new MainMenuState));
		break;
	case GameStateName::PLAY:
		m_currentState = std::move(std::unique_ptr<PlayState>(new PlayState));
		break;
	case GameStateName::QUIT:
		m_running = false;
		break;
	default:
		m_currentState = std::move(std::unique_ptr<MainMenuState>(new MainMenuState));
		break;
	}
}
