#include "PlayerInputManager.h"

#include "Game.h"
#include "Player.h"
#include "Config.h"

PlayerInputManager::PlayerInputManager() {}
PlayerInputManager::~PlayerInputManager() {}

void PlayerInputManager::handleInput(const sf::Event & event, const sf::Vector2f mousePosition, Game& game)
{

	if (event.type == sf::Event::MouseButtonPressed)
	{
		handleShot();
	}

	if (event.type == sf::Event::MouseMoved)
	{
		handleMouselook(event, game.getWindow());
	}

	//escape go to main menu
	if (event.type == sf::Event::KeyPressed)
	{
		// handle controls
		if ((event.key.code == sf::Keyboard::Left) || (event.key.code == sf::Keyboard::A))
		{
			m_stepLeft = true;
		}
		else if ((event.key.code == sf::Keyboard::Right) || (event.key.code == sf::Keyboard::D))
		{
			m_stepRight = true;
		}
		else if ((event.key.code == sf::Keyboard::Up) || (event.key.code == sf::Keyboard::W))
		{
			m_forward = true;
		}
		else if ((event.key.code == sf::Keyboard::Down) || (event.key.code == sf::Keyboard::S))
		{
			m_backward = true;
		}
	}

	if (event.type == sf::Event::KeyReleased)
	{
		// handle controls
		if ((event.key.code == sf::Keyboard::Left) || (event.key.code == sf::Keyboard::A))
		{
			m_stepLeft = false;
		}
		if ((event.key.code == sf::Keyboard::Right) || (event.key.code == sf::Keyboard::D))
		{
			m_stepRight = false;
		}
		if ((event.key.code == sf::Keyboard::Up) || (event.key.code == sf::Keyboard::W))
		{
			m_forward = false;
		}
		if ((event.key.code == sf::Keyboard::Down) || (event.key.code == sf::Keyboard::S))
		{
			m_backward = false;
		}
		if ((event.key.code == sf::Keyboard::Space) || (event.key.code == sf::Keyboard::LControl))
		{
			handleShot();
		}
	}
}

void PlayerInputManager::updatePlayerMovement(const double fts, std::shared_ptr<Player> m_player, const std::vector<std::vector<int> >& m_levelRef)
{
	calculateShotTime(fts);

	//update position
	if (isMoving())
	{
		// convert ms to seconds
		double moveSpeed = fts * 5.0; //the constant value is in squares/second
		double rotSpeed = fts * std::abs(m_mouseDelta) * g_lookSpeed;

		if (m_left)
		{
			//both camera direction and camera plane must be rotated
			double oldDirX = m_player->m_dirX;
			m_player->m_dirX = m_player->m_dirX * cos(rotSpeed) - m_player->m_dirY * sin(rotSpeed);
			m_player->m_dirY = oldDirX * sin(rotSpeed) + m_player->m_dirY * cos(rotSpeed);
			double oldPlaneX = m_player->m_planeX;
			m_player->m_planeX = m_player->m_planeX * cos(rotSpeed) - m_player->m_planeY * sin(rotSpeed);
			m_player->m_planeY = oldPlaneX * sin(rotSpeed) + m_player->m_planeY * cos(rotSpeed);
		}
		if (m_right)
		{
			//both camera direction and camera plane must be rotated
			double oldDirX = m_player->m_dirX;
			m_player->m_dirX = m_player->m_dirX * cos(-rotSpeed) - m_player->m_dirY * sin(-rotSpeed);
			m_player->m_dirY = oldDirX * sin(-rotSpeed) + m_player->m_dirY * cos(-rotSpeed);
			double oldPlaneX = m_player->m_planeX;
			m_player->m_planeX = m_player->m_planeX * cos(-rotSpeed) - m_player->m_planeY * sin(-rotSpeed);
			m_player->m_planeY = oldPlaneX * sin(-rotSpeed) + m_player->m_planeY * cos(-rotSpeed);
		}
		if (m_forward)
		{
			if (m_levelRef[int(m_player->m_posX + m_player->m_dirX * moveSpeed)][int(m_player->m_posY)] == 0) 
				m_player->m_posX += m_player->m_dirX * moveSpeed;
			if (m_levelRef[int(m_player->m_posX)][int(m_player->m_posY + m_player->m_dirY * moveSpeed)] == 0)
				m_player->m_posY += m_player->m_dirY * moveSpeed;
		}
		if (m_backward)
		{
			if (m_levelRef[int(m_player->m_posX - m_player->m_dirX * moveSpeed)][int(m_player->m_posY)] == 0) 
				m_player->m_posX -= m_player->m_dirX * moveSpeed;
			if (m_levelRef[int(m_player->m_posX)][int(m_player->m_posY - m_player->m_dirY * moveSpeed)] == 0) 
				m_player->m_posY -= m_player->m_dirY * moveSpeed;
		}
		if (m_stepLeft)
		{
			// rotate vector in 90 ccw use Vector(-y, x)
			auto dirX = -m_player->m_dirY;
			auto dirY = m_player->m_dirX;

			if (m_levelRef[int(m_player->m_posX + dirX * moveSpeed)][int(m_player->m_posY)] == 0)
				m_player->m_posX += dirX * moveSpeed;
			if (m_levelRef[int(m_player->m_posX)][int(m_player->m_posY + dirY * moveSpeed)] == 0)
				m_player->m_posY += dirY * moveSpeed;
		}
		if (m_stepRight)
		{
			// rotate vector in 90 cw use Vector(y, -x)
			auto dirX = m_player->m_dirY;
			auto dirY = -m_player->m_dirX;

			if (m_levelRef[int(m_player->m_posX + dirX * moveSpeed)][int(m_player->m_posY)] == 0)
				m_player->m_posX += dirX * moveSpeed;
			if (m_levelRef[int(m_player->m_posX)][int(m_player->m_posY + dirY * moveSpeed)] == 0)
				m_player->m_posY += dirY * moveSpeed;
		}

		//stop rotating on mouselook
		m_left = false;
		m_right = false;
	}
}

void PlayerInputManager::calculateShotTime(double fts)
{
	if (m_shotTime < 0.0)
	{
		m_shooting = false;
	}
	else
	{
		m_shotTime -= fts;
	}
	if (m_gunShotDelay > 0.0)
	{
		m_gunShotDelay -= fts;
	}
}

void PlayerInputManager::handleShot()
{
	if (m_shotTime > 0.0 || m_gunShotDelay > 0.0)
	{
		m_shooting = false;
		return;
	}
	m_shotTime = g_gunShotTime;
	m_gunShotDelay = g_gunShotDelayTime;
	m_shooting = true;
}


void PlayerInputManager::handleMouselook(const sf::Event & event, const sf::RenderWindow& window)
{
	int mouseX = event.mouseMove.x;
	int mouseY = event.mouseMove.y;

	m_mouseDelta = mouseX - m_lastMouseX;
	m_lastMouseX = mouseX;
	
	int windowX = static_cast<int>(window.getSize().x);
	int windowY = static_cast<int>(window.getSize().y);

	//if mouse is out of screen, put it to the center
	if (mouseX <= m_mouseBorder || mouseX >= windowX - m_mouseBorder ||
		mouseY <= m_mouseBorder || mouseY >= windowY - m_mouseBorder)
	{
		auto centerX = windowX / 2;
		auto centerY = windowY / 2;
		sf::Mouse::setPosition(sf::Vector2i(centerX, centerY), window);
		m_lastMouseX = centerX;
	}

	if (m_mouseDelta < 0)
	{
		m_right = false;
		m_left = true;
	}
	else if (m_mouseDelta > 0)
	{
		m_left = false;
		m_right = true;
	}
	else
	{
		m_left = false;
		m_right = false;
	}

}