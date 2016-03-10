#include "PlayState.h"

#include <algorithm>
#include <string>

PlayState::PlayState(const int w, const int h, std::shared_ptr<Player> player, std::shared_ptr<LevelReaderWriter> levelReader) :
	m_windowWidth(w), 
	m_windowHeight(h),
	m_player(move(player)),
	m_levelReader(move(levelReader)),
	m_levelRef(m_levelReader->getLevel()),
	m_spriteRef(m_levelReader->getSprites()),
	m_levelSize(m_levelReader->getLevel().size()),
	m_spriteSize(m_levelReader->getSprites().size()),
	m_mousePosition(sf::Vector2f(0.0f, 0.0f))
{
	
	m_ZBuffer.resize(w);
	m_spriteOrder.resize(m_levelReader->getSprites().size());
	m_spriteDistance.resize(m_levelReader->getSprites().size());
	m_clickables.resize(m_levelReader->getSprites().size());

	//FIXME create separate font loader
	m_font.loadFromFile(g_fontPath);
	
	//Fps display
	m_fpsDisplay.setFont(m_font);
	m_fpsDisplay.setString("fps");
	m_fpsDisplay.setCharacterSize(32);
	m_fpsDisplay.setPosition(float(w) - 10.0f, 0.0f);
	m_fpsDisplay.setColor(sf::Color::Yellow);

	//Health display
	m_playerHealthDisplay.setFont(m_font);
	m_playerHealthDisplay.setString("health");
	m_playerHealthDisplay.setCharacterSize(40);
	m_playerHealthDisplay.setPosition(10.0f, float(h) - m_playerHealthDisplay.getGlobalBounds().height * 3);
	m_playerHealthDisplay.setColor(sf::Color::White);
	
	m_buffer.resize(h * w * 3);
	m_glRenderer.init(&m_buffer[0], w, h);

	//Gun display
	//idle texture
	sf::Image gunImg;
	gunImg.loadFromFile(g_gunSprite);
	gunImg.createMaskFromColor(sf::Color::Black);
	m_textureGun.loadFromImage(gunImg);
	
	//fire texture
	sf::Image gunImgFire;
	gunImgFire.loadFromFile(g_gunSprite_fire);
	gunImgFire.createMaskFromColor(sf::Color::Black);
	m_textureGun_fire.loadFromImage(gunImgFire);
	
	//set gun size, position and texture
	m_gunDisplay.setSize({ float(g_textureWidth * 2), float(g_textureHeight * 2) });
	m_gunDisplay.setPosition({ float(w/2 - g_textureWidth), float(h - g_textureHeight*2 + 30) });
	m_gunDisplay.setTexture(&m_textureGun);

	//crosshair
	m_crosshair.setRadius(2.0f);
	m_crosshair.setPosition({ float(w / 2) - 1.0f, float(h / 2) - 1.0f });
	m_crosshair.setFillColor(sf::Color::White);


}

void PlayState::update(const float ft) {
	
	//update health each frame
	m_playerHealthDisplay.setString("+ " + std::to_string(m_player->m_health));
	
	double fts = static_cast<double>(ft / 1000);

	if(m_shotTime < 0.0){
		m_gunDisplay.setTexture(&m_textureGun);
	}
	else {
		m_shotTime -= fts;
	}
	if (m_gunShotDelay > 0.0) {
		m_gunShotDelay -= fts;
	}

	//update position
	if (m_forward || m_backward || m_left || m_right) {
		// convert ms to seconds
		double moveSpeed = fts * 5.0; //the constant value is in squares/second
		double rotSpeed = fts * 3.0; //the constant value is in radians/second

		//gun wobble
		auto wobbleSpeed = fts * 10.0f;
		auto newGunPos = m_gunDisplay.getPosition();
		float DeltaHeight = static_cast<float>(sin(m_runningTime + wobbleSpeed) - sin(m_runningTime));
		newGunPos.y += DeltaHeight * 15.0f;
		m_runningTime += wobbleSpeed;
		m_gunDisplay.setPosition(newGunPos);

		if (m_left) {
			//both camera direction and camera plane must be rotated
			double oldDirX = m_player->m_dirX;
			m_player->m_dirX = m_player->m_dirX * cos(rotSpeed) - m_player->m_dirY * sin(rotSpeed);
			m_player->m_dirY = oldDirX * sin(rotSpeed) + m_player->m_dirY * cos(rotSpeed);
			double oldPlaneX = m_player->m_planeX;
			m_player->m_planeX = m_player->m_planeX * cos(rotSpeed) - m_player->m_planeY * sin(rotSpeed);
			m_player->m_planeY = oldPlaneX * sin(rotSpeed) + m_player->m_planeY * cos(rotSpeed);
		}
		if (m_right) {
			//both camera direction and camera plane must be rotated
			double oldDirX = m_player->m_dirX;
			m_player->m_dirX = m_player->m_dirX * cos(-rotSpeed) - m_player->m_dirY * sin(-rotSpeed);
			m_player->m_dirY = oldDirX * sin(-rotSpeed) + m_player->m_dirY * cos(-rotSpeed);
			double oldPlaneX = m_player->m_planeX;
			m_player->m_planeX = m_player->m_planeX * cos(-rotSpeed) - m_player->m_planeY * sin(-rotSpeed);
			m_player->m_planeY = oldPlaneX * sin(-rotSpeed) + m_player->m_planeY * cos(-rotSpeed);
		}
		if (m_forward) {
			if (m_levelRef[int(m_player->m_posX + m_player->m_dirX * moveSpeed)][int(m_player->m_posY)] == false) m_player->m_posX += m_player->m_dirX * moveSpeed;
			if (m_levelRef[int(m_player->m_posX)][int(m_player->m_posY + m_player->m_dirY * moveSpeed)] == false) m_player->m_posY += m_player->m_dirY * moveSpeed;
		}
		if (m_backward) {
			if (m_levelRef[int(m_player->m_posX - m_player->m_dirX * moveSpeed)][int(m_player->m_posY)] == false) m_player->m_posX -= m_player->m_dirX * moveSpeed;
			if (m_levelRef[int(m_player->m_posX)][int(m_player->m_posY - m_player->m_dirY * moveSpeed)] == false) m_player->m_posY -= m_player->m_dirY * moveSpeed;
		}
	}
}

void PlayState::draw(sf::RenderWindow& window) {

	std::vector<unsigned char>().swap(m_buffer);
	m_buffer.resize(m_windowWidth * m_windowHeight * 3);

	//calculate a new buffer
	calculateWalls();
	calculateSprites();

	m_glRenderer.draw(&m_buffer[0], m_windowWidth, m_windowHeight);
	m_glRenderer.unbindBuffers();

	window.pushGLStates();

	//draw minimap
	drawMinimap(&window);

	//draw Gui elements
	drawGui(&window);

	window.popGLStates();

	m_glRenderer.bindBuffers();

	window.display();
}

void PlayState::calculateWalls() {

	const double rayPosX = m_player->m_posX;
	const double rayPosY = m_player->m_posY;

	//what direction to step in x or y-direction (either +1 or -1)
	int stepX;
	int stepY;

	//length of ray from current position to next x or y-side
	double sideDistX;
	double sideDistY;

	double perpWallDist;
	double wallX; //where exactly the wall was hit

	const std::vector<sf::Uint32>& tex8 = m_levelReader->getTexture(8);//floor
	const std::vector<sf::Uint32>& tex9 = m_levelReader->getTexture(9);//ceiling

	//2d raycaster
	for (int x = 0; x < m_windowWidth; x++)	{
		//calculate ray position and direction
		const double cameraX = 2 * x / double(m_windowWidth) - 1; //x-coordinate in camera space
		
		const double rayDirX = m_player->m_dirX + m_player->m_planeX * cameraX;
		const double rayDirY = m_player->m_dirY + m_player->m_planeY * cameraX;

		//which box of the map we're in
		int mapX = static_cast<int>(rayPosX);
		int mapY = static_cast<int>(rayPosY);

		//length of ray from one x or y-side to next x or y-side
		const double rayDirXsq = rayDirX * rayDirX;
		const double rayDirYsq = rayDirY * rayDirY;
		const double deltaDistX = sqrt(1 + rayDirYsq / rayDirXsq);
		const double deltaDistY = sqrt(1 + rayDirXsq / rayDirYsq);

		//calculate step and initial sideDist
		if (rayDirX < 0) {
			stepX = -1;
			sideDistX = (rayPosX - mapX) * deltaDistX;
		}
		else {
			stepX = 1;
			sideDistX = (mapX + 1.0 - rayPosX) * deltaDistX;
		}
		if (rayDirY < 0) {
			stepY = -1;
			sideDistY = (rayPosY - mapY) * deltaDistY;
		}
		else {
			stepY = 1;
			sideDistY = (mapY + 1.0 - rayPosY) * deltaDistY;
		}

		auto hit = 0; //was there a wall hit?
		auto side = 0; //was a NS or a EW wall hit?

		//perform DDA
		while (hit == 0) {
			//jump to next map square, OR in x-direction, OR in y-direction
			if (sideDistX < sideDistY) {
				sideDistX += deltaDistX;
				mapX += stepX;
				side = 0;
			}
			else {
				sideDistY += deltaDistY;
				mapY += stepY;
				side = 1;
			}
			//Check if ray has hit a wall
			if (m_levelRef[mapX][mapY] > 0) hit = 1;
		}

		//Calculate distance projected on camera direction (oblique distance will give fisheye effect!)
		if (side == 0) {
			perpWallDist = fabs((mapX - rayPosX + (1 - stepX) / 2) / rayDirX);
		}
		else {
			perpWallDist = fabs((mapY - rayPosY + (1 - stepY) / 2) / rayDirY);
		}

		//Calculate height of line to draw on screen
		const int lineHeight = static_cast<int>(std::abs(m_windowHeight / perpWallDist));

		//calculate lowest and highest pixel to fill in current stripe
		int drawStart = -lineHeight / 2 + m_windowHeight / 2;
		if (drawStart < 0)drawStart = 0;
		int drawEnd = lineHeight / 2 + m_windowHeight / 2;
		if (drawEnd >= m_windowHeight)drawEnd = m_windowHeight - 1;

		//texturing calculations
		const int texNum = m_levelRef[mapX][mapY] - 1; //1 subtracted from it so that texture 0 can be used!

		if (side == 1) {
			wallX = rayPosX + ((mapY - rayPosY + (1 - stepY) / 2) / rayDirY) * rayDirX;
		}
		else {
			wallX = rayPosY + ((mapX - rayPosX + (1 - stepX) / 2) / rayDirX) * rayDirY;
		}
		wallX -= floor(wallX);

		//x coordinate on the texture
		int texX = static_cast<int>(wallX * g_textureWidth);
		if (side == 0 && rayDirX > 0) texX = g_textureWidth - texX - 1;
		if (side == 1 && rayDirY < 0) texX = g_textureWidth - texX - 1;

		const std::vector<sf::Uint32>& texture = m_levelReader->getTexture(texNum);
		const size_t texSize = texture.size();

		//darker sides
		for (int y = drawStart; y < drawEnd; y++) {

			const int d = y * 256 - m_windowHeight * 128 + lineHeight * 128;  //256 and 128 factors to avoid floats
			const int texY = ((d * g_textureHeight) / lineHeight) / 256;
			const unsigned int texNumY = g_textureHeight * texX + texY;

			if (texNumY < texSize) {
				auto color = texture[texNumY];
				setPixel(x, y, color, side==1);
			}
		}

		//SET THE ZBUFFER FOR THE SPRITE CASTING
		m_ZBuffer[x] = perpWallDist; //perpendicular distance is used

		//FLOOR CASTING
		double floorXWall, floorYWall; //x, y position of the floor texel at the bottom of the wall

		//4 different wall directions possible
		if (side == 0 && rayDirX > 0) {
			floorXWall = mapX;
			floorYWall = mapY + wallX;
		}
		else if (side == 0 && rayDirX < 0) {
			floorXWall = mapX + 1.0;
			floorYWall = mapY + wallX;
		}
		else if (side == 1 && rayDirY > 0) {
			floorXWall = mapX + wallX;
			floorYWall = mapY;
		}
		else {
			floorXWall = mapX + wallX;
			floorYWall = mapY + 1.0;
		}

		if (drawEnd < 0) drawEnd = m_windowHeight; //becomes < 0 when the integer overflows
		
		//draw the floor from drawEnd to the bottom of the screen
		for (int y = drawEnd + 1; y < m_windowHeight; y++) {

			const double currentDist = m_windowHeight / (2.0 * y - m_windowHeight); //you could make a small lookup table for this instead
			const double weight = currentDist / perpWallDist;

			const double currentFloorX = weight * floorXWall + (1.0 - weight) * m_player->m_posX;
			const double currentFloorY = weight * floorYWall + (1.0 - weight) * m_player->m_posY;

			const int floorTexX = int(currentFloorX * g_textureWidth) % g_textureWidth;
			const int floorTexY = int(currentFloorY * g_textureHeight) % g_textureHeight;

			//floor textures
			sf::Uint32 color1 = tex8[g_textureWidth * floorTexY + floorTexX];
			sf::Uint32 color2 = tex9[g_textureWidth * floorTexY + floorTexX];

			setPixel(x, y, color1, false);
			setPixel(x, m_windowHeight - y, color2, false);
		}
	}

}

void PlayState::calculateSprites() {

	//SPRITE CASTING
	//sort sprites from far to close
	for (size_t i = 0; i < m_spriteSize; i++) {
		m_spriteOrder[i] = i;
		m_spriteDistance[i] = ((m_player->m_posX - m_spriteRef[i].x) * (m_player->m_posX - m_spriteRef[i].x) + (m_player->m_posY - m_spriteRef[i].y) * (m_player->m_posY - m_spriteRef[i].y)); //sqrt not taken, unneeded
	}
	combSort(m_spriteOrder, m_spriteDistance, m_spriteRef.size());

	//after sorting the sprites, do the projection and draw them
	for (size_t i = 0; i < m_spriteRef.size(); i++) {
		
		//translate sprite position to relative to camera
		const double spriteX = m_spriteRef[m_spriteOrder[i]].x - m_player->m_posX;
		const double spriteY = m_spriteRef[m_spriteOrder[i]].y - m_player->m_posY;

		const double invDet = 1.0 / (m_player->m_planeX * m_player->m_dirY - m_player->m_dirX * m_player->m_planeY); //required for correct matrix multiplication
		const double transformX = invDet * (m_player->m_dirY * spriteX - m_player->m_dirX * spriteY);
		const double transformY = invDet * (-m_player->m_planeY * spriteX + m_player->m_planeX * spriteY); //this is actually the depth inside the screen, that what Z is in 3D       
		const int spriteScreenX = int((m_windowWidth / 2) * (1 + transformX / transformY));

		//calculate height of the sprite on screen
		const int spriteHeight = abs(int(m_windowHeight / (transformY))); //using "transformY" instead of the real distance prevents fisheye

		//calculate lowest and highest pixel to fill in current stripe
		int drawStartY = -spriteHeight / 2 + m_windowHeight / 2;
		int drawEndY = spriteHeight / 2 + m_windowHeight / 2;
		
		//calculate width of the sprite
		int spriteWidth = spriteHeight;
		int drawStartX = -spriteWidth / 2 + spriteScreenX;
		int drawEndX = spriteWidth / 2 + spriteScreenX;

		const int texNr = m_spriteRef[m_spriteOrder[i]].texture;
		const std::vector<sf::Uint32>& textureData = m_levelReader->getTexture(texNr);
		const int texSize = textureData.size();
		
		//setup clickables
		m_clickables[i].update(sf::Vector2f(float(spriteWidth / 2.0f), float(spriteHeight)), sf::Vector2f(float(drawStartX + spriteWidth / 4.0f), float(drawStartY)));
		m_clickables[i].setSpriteIndex(m_spriteOrder[i]);

		//limit drawstart and drawend
		if (drawStartY < 0) drawStartY = 0;
		if (drawEndY >= m_windowHeight) drawEndY = m_windowHeight - 1;
		if (drawStartX < 0) drawStartX = 0;
		if (drawEndX >= m_windowWidth) drawEndX = m_windowWidth - 1;

		//loop through every vertical stripe of the sprite on screen
		for (int stripe = drawStartX; stripe < drawEndX; stripe++) {
			
			const int texX = int(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) * g_textureWidth / spriteWidth) / 256;

			//the conditions in the if are:
			//1) it's in front of camera plane so you don't see things behind you
			//2) it's on the screen (left)
			//3) it's on the screen (right)
			//4) ZBuffer, with perpendicular distance
			if (transformY > 0 && stripe > 0 && stripe < m_windowWidth && transformY < m_ZBuffer[stripe]) {

				//for every pixel of the current stripe
				for (int y = drawStartY; y < drawEndY; y++) {

					m_clickables[i].setVisible(texNr != 12);
					m_clickables[i].setDestructible(texNr != 12);

					const int d = (y)* 256 - m_windowHeight * 128 + spriteHeight * 128; //256 and 128 factors to avoid floats
					const int texY = ((d * g_textureHeight) / spriteHeight) / 256;
					const int texPix = g_textureWidth * texX + texY;

					// prevent exception when accessing tex pixel out of range
					if (texPix < texSize) {
						sf::Uint32 color = textureData[texPix]; //get current color from the texture

																// black is invisible!!!
						if ((color & 0x00FFFFFF) != 0) {
							//brighten if mouse over
							setPixel(stripe, y, color, 0);
						}
					}
				}

			}
		}

	}
}

//Render minimap
void PlayState::drawMinimap(sf::RenderWindow* window) const {

	//minimap background
	sf::RectangleShape minimapBg(sf::Vector2f(m_levelSize * g_playMinimapScale, m_levelSize * g_playMinimapScale));
	minimapBg.setPosition(0, 0);
	minimapBg.setFillColor(sf::Color(150, 150, 150, g_playMinimapTransparency));
	window->draw(minimapBg);

	//draw walls
	for (size_t y = 0; y < m_levelSize; y++) {
		for (size_t x = 0; x < m_levelSize; x++) {
			if (m_levelRef[y][x] > 0 && m_levelRef[y][x] < 9) {
				sf::RectangleShape wall(sf::Vector2f(g_playMinimapScale, g_playMinimapScale));
				wall.setPosition(x * g_playMinimapScale, y * g_playMinimapScale);
				wall.setFillColor(sf::Color(0, 0, 0, g_playMinimapTransparency));
				window->draw(wall);
			}
		}
	}

	// Render entities on minimap
	for (size_t i = 0; i < m_spriteSize; i++) {
		sf::CircleShape object(g_playMinimapScale / 4.0f);
		object.setPosition(float(m_spriteRef[i].y) * g_playMinimapScale, float(m_spriteRef[i].x) * g_playMinimapScale);
		object.setOrigin(g_playMinimapScale / 2.0f, g_playMinimapScale / 2.0f);
		object.setFillColor(sf::Color(0, 0, 255, g_playMinimapTransparency));
		window->draw(object);
	}
	
	// Render Player on minimap
	{
		sf::CircleShape player(float(g_playMinimapScale), 3);
		player.setPosition(float(m_player->m_posY) * g_playMinimapScale, float(m_player->m_posX) * g_playMinimapScale);
		player.setFillColor(sf::Color(255, 255, 255, g_playMinimapTransparency));
		player.setOrigin(float(g_playMinimapScale), float(g_playMinimapScale));

		sf::RectangleShape player2(sf::Vector2f(g_playMinimapScale / 2.0f, g_playMinimapScale / 2.0f));
		player2.setPosition(float(m_player->m_posY) * g_playMinimapScale, float(m_player->m_posX) * g_playMinimapScale);
		player2.setFillColor(sf::Color(255, 255, 255, g_playMinimapTransparency));
		player2.setOrigin(g_playMinimapScale / 4.0f, -g_playMinimapScale / 2.0f);

		float angle = std::atan2f(float(m_player->m_dirX), float(m_player->m_dirY));
		player.setRotation((angle * 57.2957795f) + 90);
		player2.setRotation((angle * 57.2957795f) + 90);

		window->draw(player);
		window->draw(player2);
	}
}

void PlayState::drawGui(sf::RenderWindow* window) {
	
	//draw hud clickable items
	for (auto& outline : m_clickables) {
		if (outline.containsVector(m_crosshair.getPosition())) {
			outline.draw(window);
			break;
		}
	}
	for (auto& outline : m_clickables) {
		outline.setVisible(false);
	}

	//draw gun
	window->draw(m_gunDisplay);

	//draw fps display
	window->draw(m_fpsDisplay);

	//draw player health
	window->draw(m_playerHealthDisplay);

	//draw crosshair
	window->draw(m_crosshair);
}

void PlayState::setPixel(int x, int y, const sf::Uint32 colorRgba, int style) {

	if (x >= m_windowWidth || y >= m_windowHeight) {
		return;
	}

	auto colors = (sf::Uint8*)&colorRgba;
	auto index = (y * m_windowWidth + x) * 3;

	if (style == g_playDrawDarkened) {
		m_buffer[index] = colors[0] / 2;
		m_buffer[index + 1] = colors[1] / 2;
		m_buffer[index + 2] = colors[2] / 2;
	}
	else if (style == g_playhDrawHighlighted) {
		m_buffer[index] = std::min(colors[0] + 25, 255);
		m_buffer[index + 1] = std::min(colors[1] + 25, 255);
		m_buffer[index + 2] = std::min(colors[2] + 25, 255);
	}
	else {
		m_buffer[index] = colors[0];
		m_buffer[index + 1] = colors[1];
		m_buffer[index + 2] = colors[2];
	}
}

void PlayState::handleShot(){

	if (m_shotTime > 0.0 || m_gunShotDelay > 0.0) return;

	m_gunDisplay.setTexture(&m_textureGun_fire);
	m_shotTime = g_gunShotTime;
	m_gunShotDelay = g_gunShotDelayTime;
	for (auto& outline : m_clickables) {
		if (outline.containsVector(m_crosshair.getPosition()) && outline.getDestructible()) {
			outline.setDestructible(false);
			m_levelReader->deleteSprite(outline.getSpriteIndex());
			break;
		}
	}
}

void PlayState::handleInput(const sf::Event & event, const sf::Vector2f & mousepPosition, Game & game) {

	m_mousePosition = mousepPosition;

	//update fps from game
	m_fpsDisplay.setString(std::to_string(game.getFps()));
	m_fpsDisplay.setOrigin(m_fpsDisplay.getGlobalBounds().width, 0.0f);

	if (event.type == sf::Event::MouseButtonPressed) {
		handleShot();
	}

	//escape go to main menu
	if (event.type == sf::Event::KeyPressed) {
		// handle controls
		if ((event.key.code == sf::Keyboard::Left) || (event.key.code == sf::Keyboard::A)) {
			m_left = true;
		}
		else if ((event.key.code == sf::Keyboard::Right) || (event.key.code == sf::Keyboard::D)) {
			m_right = true;
		}
		else if ((event.key.code == sf::Keyboard::Up) || (event.key.code == sf::Keyboard::W)) {
			m_forward = true;
		}
		else if ((event.key.code == sf::Keyboard::Down) || (event.key.code == sf::Keyboard::S))	{
			m_backward = true;
		}
		else if ((event.key.code == sf::Keyboard::Space) || (event.key.code == sf::Keyboard::LControl)) {
			handleShot();
		}

	}

	if (event.type == sf::Event::KeyReleased) {
		// handle controls
		if ((event.key.code == sf::Keyboard::Left) || (event.key.code == sf::Keyboard::A)) {
			m_left = false;
		}
		if ((event.key.code == sf::Keyboard::Right) || (event.key.code == sf::Keyboard::D)) {
			m_right = false;
		}
		if ((event.key.code == sf::Keyboard::Up) || (event.key.code == sf::Keyboard::W)) {
			m_forward = false;
		}
		if ((event.key.code == sf::Keyboard::Down) || (event.key.code == sf::Keyboard::S)) {
			m_backward = false;
		}
		if (event.key.code == sf::Keyboard::Escape)	{
			m_glRenderer.cleanup();
			game.changeState(GameStateName::MAINMENU);
		}
	}
}

//sort algorithm
void PlayState::combSort(std::vector<int>& order, std::vector<double>& dist, int amount) const {
	
	int gap = amount;
	bool swapped = false;
	while (gap > 1 || swapped) {
		//shrink factor 1.3
		gap = (gap * 10) / 13;
		if (gap == 9 || gap == 10) gap = 11;
		if (gap < 1) gap = 1;
		swapped = false;
		for (int i = 0; i < amount - gap; i++) {
			int j = i + gap;
			if (dist[i] < dist[j]) {
				std::swap(dist[i], dist[j]);
				std::swap(order[i], order[j]);
				swapped = true;
			}
		}
	}
}
