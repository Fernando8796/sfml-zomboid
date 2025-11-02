#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <algorithm>
#include <limits>
#include <cstdlib> // Para números aleatórios (rand)
#include <ctime>   // Para a semente de números aleatórios (time)
#include <string>  // Para o texto de Game Over e HUD
#include <sstream> // Para converter int para string

// NOVO: Estados do Jogo
enum GameState {
    MainMenu,
    Playing,
    GameOverScreen
};

// Estrutura da Bala
struct Bullet {
    sf::CircleShape shape;
    sf::Vector2f velocity;
};

// Estrutura do Zumbi (com sf::Sprite e float radius)
struct Zombie {
    sf::Sprite sprite; 
    float radius;      
};

// Função para verificar colisão entre dois círculos
bool checkCircleCollision(sf::Vector2f p1, float r1, sf::Vector2f p2, float r2) {
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;
    float distanceSquared = dx * dx + dy * dy;
    float radiusSum = r1 + r2;
    return distanceSquared < (radiusSum * radiusSum);
}

// Variáveis globais do estado da wave
int currentWave = 0;
int zombiesToSpawn = 0;
int zombiesSpawnedThisWave = 0;
int zombiesRemaining = 0;
const int INITIAL_ZOMBIES = 10;
const int ZOMBIE_INCREMENT_PER_WAVE = 5;

// Função para reiniciar o jogo (sem 'gameOver')
void resetGame(bool& p1Alive, bool& p2Alive, // Removido gameOver
               sf::CircleShape& player1, sf::CircleShape& player2,
               sf::RectangleShape& base, 
               std::vector<Zombie>& zombies, std::vector<Bullet>& bullets,
               sf::Clock& spawnClock, unsigned int worldW, unsigned int worldH) 
{
    p1Alive = true;
    p2Alive = true;
    // gameOver não é mais necessário aqui
    
    player1.setPosition(worldW / 3.0f, worldH / 2.0f);
    player2.setPosition(2 * worldW / 3.0f, worldH / 2.0f);
    base.setPosition(worldW / 2.0f, worldH / 2.0f); 
    
    zombies.clear();
    bullets.clear();
    spawnClock.restart();

    // Reseta variáveis do sistema de waves
    currentWave = 0; 
    zombiesToSpawn = 0; 
    zombiesSpawnedThisWave = 0;
    zombiesRemaining = 0;
}

// Função para iniciar a próxima wave
void startNextWave() {
    currentWave++;
    zombiesToSpawn = INITIAL_ZOMBIES + (currentWave - 1) * ZOMBIE_INCREMENT_PER_WAVE;
    zombiesSpawnedThisWave = 0;
    zombiesRemaining = zombiesToSpawn; 
}


int main() {
    srand(static_cast<unsigned>(time(0)));

    // Configurações da Janela e Mundo
    const unsigned int WINDOW_W = 800; 
    const unsigned int WINDOW_H = 600; 
    const unsigned int WORLD_W = 1600; 
    const unsigned int WORLD_H = 1200; 

    // Constantes
    const float SPEED = 300.0f;
    const float BULLET_SPEED = 600.0f; 
    const float ZOMBIE_SPEED = 60.0f; 
    const float ZOMBIE_SPAWN_TIME = 0.5f; 
    const float BULLET_RADIUS = 5.f; 
    const float BULLET_RATE = 200; 

    sf::RenderWindow window(sf::VideoMode(WINDOW_W, WINDOW_H), "SFML Zomboid"); // Título da Janela
    window.setFramerateLimit(144);

    // Câmera (View)
    sf::View gameView(sf::FloatRect(0, 0, (float)WINDOW_W, (float)WINDOW_H));
    window.setView(gameView); 

    // Fundo do mapa
    sf::RectangleShape background(sf::Vector2f((float)WORLD_W, (float)WORLD_H));
    background.setFillColor(sf::Color(40, 40, 40)); 

    // Base Central
    sf::RectangleShape base(sf::Vector2f(24.f, 24.f)); 
    base.setFillColor(sf::Color::Yellow);
    base.setOrigin(12.f, 12.f);
    base.setPosition(WORLD_W / 2.0f, WORLD_H / 2.0f); 

    // Carrega Fonte
    sf::Font font;
    if (!font.loadFromFile("zombie.otf")) { 
        return -1; // Erro fatal se a fonte não carregar
    }

    // --- NOVO: Textos do Main Menu ---
    sf::Text titleText;
    titleText.setFont(font);
    titleText.setString("SFML Zomboid");
    titleText.setCharacterSize(70);
    titleText.setFillColor(sf::Color::Green);
    sf::FloatRect titleRect = titleText.getLocalBounds();
    titleText.setOrigin(titleRect.left + titleRect.width / 2.0f, titleRect.top + titleRect.height / 2.0f);
    titleText.setPosition(WINDOW_W / 2.0f, WINDOW_H / 2.0f - 100.f);

    sf::Text playButtonText;
    playButtonText.setFont(font);
    playButtonText.setString("Pressione ENTER para Jogar");
    playButtonText.setCharacterSize(35);
    playButtonText.setFillColor(sf::Color::White);
    sf::FloatRect playRect = playButtonText.getLocalBounds();
    playButtonText.setOrigin(playRect.left + playRect.width / 2.0f, playRect.top + playRect.height / 2.0f);
    playButtonText.setPosition(WINDOW_W / 2.0f, WINDOW_H / 2.0f + 50.f);

    // Texto de Game Over
    sf::Text gameOverText;
    gameOverText.setFont(font);
    gameOverText.setCharacterSize(50);
    gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setString("GAME OVER!\nPressione 'R' para reiniciar.");
    sf::FloatRect textRect = gameOverText.getLocalBounds();
    gameOverText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
    gameOverText.setPosition(WINDOW_W / 2.0f, WINDOW_H / 2.0f); 

    // Textos do HUD (Wave e Zumbis)
    sf::Text waveText;
    waveText.setFont(font);
    waveText.setCharacterSize(30);
    waveText.setFillColor(sf::Color::White);

    sf::Text zombiesRemainingText;
    zombiesRemainingText.setFont(font);
    zombiesRemainingText.setCharacterSize(25);
    zombiesRemainingText.setFillColor(sf::Color::White);

    // Carrega Textura do Zumbi
    sf::Texture zombieTexture;
    if (!zombieTexture.loadFromFile("zombie.png")) {
        return -1; 
    }

    // Players
    sf::CircleShape player1(12.0f);
    player1.setFillColor(sf::Color::Red); 
    player1.setOrigin(player1.getRadius(), player1.getRadius());
    
    sf::CircleShape player2(12.0f);
    player2.setFillColor(sf::Color::Blue);
    player2.setOrigin(player2.getRadius(), player2.getRadius());
    
    // Vetores
    std::vector<Bullet> bullets;
    std::vector<Zombie> zombies;
    
    // Clocks
    sf::Clock clock;
    sf::Clock shootClock1;
    sf::Clock shootClock2;
    sf::Clock zombieSpawnClock; 

    // Últimas direções
    sf::Vector2f lastDir1 = {0.f, -1.f};
    sf::Vector2f lastDir2 = {0.f, -1.f};

    // Estados dos Players
    bool player1Alive = true;
    bool player2Alive = true;

    // NOVO: Estado inicial do Jogo
    GameState currentState = MainMenu;

    // Seta a posição inicial (sem iniciar o jogo)
    resetGame(player1Alive, player2Alive, player1, player2, base, zombies, bullets, zombieSpawnClock, WORLD_W, WORLD_H);
    // startNextWave() será chamado na transição do menu para o jogo
    
    // LOOP PRINCIPAL
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::Resized) {
                sf::FloatRect visibleArea(0, 0, (float)event.size.width, (float)event.size.height);
                gameView.setSize(event.size.width, event.size.height);
                // Não precisa setar a view aqui, ela será setada no estado 'Playing'
            }

            // NOVO: Lógica de eventos para transição de estado
            if (event.type == sf::Event::KeyPressed) {
                // Se estamos no Menu Principal e "Enter" é pressionado
                if (currentState == MainMenu && event.key.code == sf::Keyboard::Enter) {
                    currentState = Playing;
                    resetGame(player1Alive, player2Alive, player1, player2, base, zombies, bullets, zombieSpawnClock, WORLD_W, WORLD_H);
                    startNextWave(); // Começa a primeira wave
                }
                
                // Se estamos no Game Over e "R" é pressionado
                if (currentState == GameOverScreen && event.key.code == sf::Keyboard::R) {
                    currentState = Playing;
                    resetGame(player1Alive, player2Alive, player1, player2, base, zombies, bullets, zombieSpawnClock, WORLD_W, WORLD_H);
                    startNextWave(); // Começa a primeira wave novamente
                }
            }
        }

        float dt = clock.restart().asSeconds();

        // Limpa a janela (será feito em cada estado)
        window.clear();

        // --- NOVO: Switch principal dos estados do jogo ---
        switch (currentState) {
            
            // --- ESTADO: MAIN MENU ---
            case MainMenu:
                window.setView(window.getDefaultView()); // Usa a view padrão (fixa)
                window.clear(sf::Color(10, 10, 30)); // Fundo azul escuro

                // Desenha Título e Botão
                window.draw(titleText);
                window.draw(playButtonText);
                break; // Fim do case MainMenu

            // --- ESTADO: PLAYING ---
            case Playing:
            { // Adiciona chaves para criar escopo local para viewCenter
                // LÓGICA DA CÂMERA (VIEW)
                sf::Vector2f viewCenter;
                if (player1Alive && player2Alive) {
                    viewCenter.x = (player1.getPosition().x + player2.getPosition().x) / 2.f;
                    viewCenter.y = (player1.getPosition().y + player2.getPosition().y) / 2.f;
                } else if (player1Alive) {
                    viewCenter = player1.getPosition();
                } else if (player2Alive) {
                    viewCenter = player2.getPosition();
                } else {
                    viewCenter = base.getPosition(); 
                }

                float halfViewW = gameView.getSize().x / 2.f;
                float halfViewH = gameView.getSize().y / 2.f;
                viewCenter.x = std::clamp(viewCenter.x, halfViewW, (float)WORLD_W - halfViewW);
                viewCenter.y = std::clamp(viewCenter.y, halfViewH, (float)WORLD_H - halfViewH);
                
                gameView.setCenter(viewCenter);
                window.setView(gameView); // Aplica a view do jogo

                // LÓGICA DO JOGO
                // Lógica de Spawn de Zumbis
                if (zombiesSpawnedThisWave < zombiesToSpawn) { 
                    if (zombieSpawnClock.getElapsedTime().asSeconds() > ZOMBIE_SPAWN_TIME) {
                        Zombie newZombie;
                        newZombie.sprite.setTexture(zombieTexture);
                        
                        float targetSize = 24.f; 
                        float originalSize = 64.f; 
                        float scale = targetSize / originalSize; 
                        
                        newZombie.sprite.setScale(scale, scale);
                        newZombie.sprite.setOrigin(originalSize / 2.f, originalSize / 2.f); 
                        newZombie.radius = targetSize / 2.f; 
                        
                        int side = rand() % 4;
                        float spawnX = 0, spawnY = 0;
                        float spawnMargin = 60.f; 

                        switch (side) {
                            case 0: spawnX = static_cast<float>(rand() % WORLD_W); spawnY = -spawnMargin; break;
                            case 1: spawnX = static_cast<float>(rand() % WORLD_W); spawnY = static_cast<float>(WORLD_H) + spawnMargin; break;
                            case 2: spawnX = -spawnMargin; spawnY = static_cast<float>(rand() % WORLD_H); break;
                            case 3: spawnX = static_cast<float>(WORLD_W) + spawnMargin; spawnY = static_cast<float>(rand() % WORLD_H); break;
                        }
                        newZombie.sprite.setPosition(spawnX, spawnY);
                        zombies.push_back(newZombie);
                        zombieSpawnClock.restart();
                        zombiesSpawnedThisWave++; 
                    }
                } else {
                    if (zombies.empty() && zombiesRemaining <= 0) {
                         startNextWave();
                    }
                }

                // Player 1 movement (WASD)
                if (player1Alive) {
                    sf::Vector2f dir1(0.f, 0.f);
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) dir1.y -= 1.f;
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) dir1.y += 1.f;
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) dir1.x -= 1.f;
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) dir1.x += 1.f;
                    
                    if (dir1.x != 0.f || dir1.y != 0.f) {
                        float len = std::sqrt(dir1.x * dir1.x + dir1.y * dir1.y);
                        dir1 /= len;
                        lastDir1 = dir1;
                    }
                    
                    sf::Vector2f pos1 = player1.getPosition() + dir1 * SPEED * dt;
                    float r1 = player1.getRadius();
                    pos1.x = std::clamp(pos1.x, r1, (float)WORLD_W - r1); 
                    pos1.y = std::clamp(pos1.y, r1, (float)WORLD_H - r1); 
                    player1.setPosition(pos1);

                    // Player 1 shooting (F)
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) {
                        if (shootClock1.getElapsedTime().asMilliseconds() > BULLET_RATE) {
                            Bullet b;
                            b.shape.setRadius(BULLET_RADIUS); 
                            b.shape.setFillColor(player1.getFillColor());
                            b.shape.setOrigin(BULLET_RADIUS / 2.f, BULLET_RADIUS / 2.f); 
                            b.shape.setPosition(player1.getPosition());
                            b.velocity = lastDir1 * BULLET_SPEED; 
                            bullets.push_back(b);
                            shootClock1.restart();
                        }
                    }
                }

                // Player 2 movement (Arrow keys)
                if (player2Alive) {
                    sf::Vector2f dir2(0.f, 0.f);
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) dir2.y -= 1.f;
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) dir2.y += 1.f;
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) dir2.x -= 1.f;
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) dir2.x += 1.f;
                    
                    if (dir2.x != 0.f || dir2.y != 0.f) {
                        float len = std::sqrt(dir2.x * dir2.x + dir2.y * dir2.y);
                        dir2 /= len;
                        lastDir2 = dir2;
                    }
                    
                    sf::Vector2f pos2 = player2.getPosition() + dir2 * SPEED * dt;
                    float r2 = player2.getRadius();
                    pos2.x = std::clamp(pos2.x, r2, (float)WORLD_W - r2); 
                    pos2.y = std::clamp(pos2.y, r2, (float)WORLD_H - r2); 
                    player2.setPosition(pos2);

                    // Player 2 shooting (Numpad0)
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad0)) {
                        if (shootClock2.getElapsedTime().asMilliseconds() > BULLET_RATE) {
                            Bullet b;
                            b.shape.setRadius(BULLET_RADIUS); 
                            b.shape.setFillColor(player2.getFillColor());
                            b.shape.setOrigin(BULLET_RADIUS / 2.f, BULLET_RADIUS / 2.f); 
                            b.shape.setPosition(player2.getPosition());
                            b.velocity = lastDir2 * BULLET_SPEED; 
                            bullets.push_back(b);
                            shootClock2.restart();
                        }
                    }
                }

                // Movimento dos Zumbis
                sf::Vector2f targetPos = base.getPosition();
                for (auto& z : zombies) {
                    sf::Vector2f zPos = z.sprite.getPosition(); 
                    if (targetPos != zPos) {
                        sf::Vector2f zombieDir = targetPos - zPos;
                        float len = std::hypot(zombieDir.x, zombieDir.y);
                        if (len > 0) zombieDir /= len;
                        z.sprite.move(zombieDir * ZOMBIE_SPEED * dt); 
                    }
                }

                // Atualiza balas
                for (auto& b : bullets)
                    b.shape.move(b.velocity * dt);

                // Remove balas fora da tela
                bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [&](const Bullet& b) {
                    auto p = b.shape.getPosition();
                    return (p.y < -100 || p.y > WORLD_H + 100 || p.x < -100 || p.x > WORLD_W + 100); 
                }), bullets.end());

                // COLISÕES
                // Colisão Balas vs Zumbis
                for (int i = bullets.size() - 1; i >= 0; --i) {
                    for (int j = zombies.size() - 1; j >= 0; --j) {
                        if (checkCircleCollision(bullets[i].shape.getPosition(), bullets[i].shape.getRadius(),
                                                 zombies[j].sprite.getPosition(), zombies[j].radius)) {
                            bullets.erase(bullets.begin() + i);
                            zombies.erase(zombies.begin() + j);
                            zombiesRemaining--; 
                            break; 
                        }
                    }
                }

                // Colisão Zumbis vs Base (GAME OVER)
                float baseRadius = base.getSize().x / 2.f; 
                for (auto& z : zombies) {
                    if (checkCircleCollision(z.sprite.getPosition(), z.radius, 
                                             base.getPosition(), baseRadius)) {
                        currentState = GameOverScreen; // NOVO: Muda o estado
                        zombiesRemaining--; 
                        break; 
                    }
                }

                // Colisão Zumbi vs Players
                if (currentState == Playing) { // Só checa se o jogo não acabou de mudar
                    for (auto& z : zombies) {
                        if (player1Alive && checkCircleCollision(z.sprite.getPosition(), z.radius, 
                                                                 player1.getPosition(), player1.getRadius())) {
                            player1Alive = false;
                        }
                        if (player2Alive && checkCircleCollision(z.sprite.getPosition(), z.radius, 
                                                                 player2.getPosition(), player2.getRadius())) {
                            player2Alive = false;
                        }
                    }
                }

                // ATUALIZA TEXTOS DO HUD
                std::stringstream ssWave;
                ssWave << "Wave: " << currentWave;
                waveText.setString(ssWave.str());
                waveText.setPosition(viewCenter.x, viewCenter.y - gameView.getSize().y / 2.f + 30.f); 
                waveText.setOrigin(waveText.getLocalBounds().width / 2.f, waveText.getLocalBounds().height / 2.f);

                std::stringstream ssZombies;
                ssZombies << "Zumbis restantes: " << zombiesRemaining;
                zombiesRemainingText.setString(ssZombies.str());
                zombiesRemainingText.setPosition(viewCenter.x, viewCenter.y - gameView.getSize().y / 2.f + 60.f);
                zombiesRemainingText.setOrigin(zombiesRemainingText.getLocalBounds().width / 2.f, zombiesRemainingText.getLocalBounds().height / 2.f);
                
                // RENDERIZAÇÃO
                window.clear(sf::Color(20, 20, 20)); 
                window.draw(background); 
                window.draw(base); 
                if (player1Alive) window.draw(player1);
                if (player2Alive) window.draw(player2);
                for (auto& z : zombies) window.draw(z.sprite); 
                for (auto& b : bullets) window.draw(b.shape);
                window.draw(waveText);
                window.draw(zombiesRemainingText);
                break; // Fim do case Playing
            }

            // --- ESTADO: GAME OVER ---
            case GameOverScreen:
                window.setView(window.getDefaultView()); // Usa a view padrão (fixa)
                window.clear(sf::Color(30, 0, 0)); // Fundo vermelho escuro
                window.draw(gameOverText);
                break; // Fim do case GameOverScreen
        }
        
        // Exibe o frame final
        window.display();
    }

    return 0;
}
