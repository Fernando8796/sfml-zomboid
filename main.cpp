#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <algorithm>
#include <limits>
#include <cstdlib> // Para números aleatórios (rand)
#include <ctime>   // Para a semente de números aleatórios (time)
#include <string>  // Para o texto
#include <sstream> // Para converter int para string

// M_PI não é padrão C++, mas geralmente está disponível ou pode ser definido
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// NOVO: Estados do Jogo
enum GameState {
    MainMenu,
    Playing,
    Paused,
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

// Estrutura da Barricada (para Player 2)
struct Barricade {
    sf::RectangleShape shape;
    int health;
    int maxHealth; // NOVO: Para exibir x/y vida
    sf::Text healthText; // NOVO: Para exibir a vida
};

// NOVO: Estrutura da Explosão (para Player 1)
struct Explosion {
    sf::CircleShape shape;
    sf::Vector2f position;
    float currentRadius;
    float maxRadius;
    float expandSpeed;
    float fadeSpeed;
    bool active;
    bool damageDealt; // Para garantir que o dano é aplicado apenas uma vez
};

// Função para verificar colisão entre dois círculos
bool checkCircleCollision(sf::Vector2f p1, float r1, sf::Vector2f p2, float r2) {
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;
    float distanceSquared = dx * dx + dy * dy;
    float radiusSum = r1 + r2;
    return distanceSquared < (radiusSum * radiusSum);
}

// Função para verificar colisão entre círculo e retângulo
bool checkCircleRectCollision(sf::Vector2f circlePos, float circleRadius,
                              sf::Vector2f rectPos, sf::Vector2f rectSize) {
    // A posição do shape do retângulo é o centro devido ao setOrigin
    float halfRectW = rectSize.x / 2.0f;
    float halfRectH = rectSize.y / 2.0f;

    float rectLeft = rectPos.x - halfRectW;
    float rectRight = rectPos.x + halfRectW;
    float rectTop = rectPos.y - halfRectH;
    float rectBottom = rectPos.y + halfRectH;

    float testX = circlePos.x;
    float testY = circlePos.y;

    if (circlePos.x < rectLeft)         testX = rectLeft;
    else if (circlePos.x > rectRight) testX = rectRight;

    if (circlePos.y < rectTop)         testY = rectTop;
    else if (circlePos.y > rectBottom) testY = rectBottom;

    float distX = circlePos.x - testX;
    float distY = circlePos.y - testY;
    float distanceSquared = (distX * distX) + (distY * distY);

    return distanceSquared < (circleRadius * circleRadius);
}


// Variáveis globais do estado da wave
int currentWave = 0;
int zombiesToSpawn = 0;
int zombiesSpawnedThisWave = 0;
int zombiesRemaining = 0;
const int INITIAL_ZOMBIES = 10;
const int ZOMBIE_INCREMENT_PER_WAVE = 5;

// Variáveis globais para o sistema de poderes
// Clocks para o cooldown
sf::Clock p1AbilityCooldownClock;
sf::Clock p2AbilityCooldownClock;

// NOVO: Variável da Explosão
Explosion p1Explosion;

std::vector<Barricade> barricades; // Vetor para armazenar as barricadas

// Função para reiniciar o jogo (apenas estado do jogo, sem reiniciar cooldowns/habilidades)
void resetGame(bool& p1Alive, bool& p2Alive,
               sf::CircleShape& player1, sf::CircleShape& player2,
               sf::RectangleShape& base, 
               std::vector<Zombie>& zombies, std::vector<Bullet>& bullets,
               sf::Clock& spawnClock, unsigned int worldW, unsigned int worldH) 
{
    p1Alive = true;
    p2Alive = true;
    
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

    // Apenas limpa barricadas e reseta explosão, cooldowns são reiniciados apenas na transição de estado
    barricades.clear();
    p1Explosion.active = false;
    p1Explosion.damageDealt = false;
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

    // Constantes do Jogo
    const float SPEED = 300.0f;
    const float BULLET_SPEED = 600.0f; 
    const float ZOMBIE_SPEED = 60.0f; 
    const float ZOMBIE_SPAWN_TIME = 0.5f; 
    const float BULLET_RADIUS = 5.f; 
    const float BULLET_RATE = 200; 
    
    // Cooldowns das Habilidades (em segundos)
    const float PLAYER1_ABILITY_COOLDOWN = 10.0f; 
    const float PLAYER2_ABILITY_COOLDOWN = 10.0f; 

    // NOVO: Constantes de Habilidade
    const int BARRIER_LIFE = 500; // Vida inicial da barricada
    const sf::Vector2f BARRICADE_SIZE = {40.f, 40.f}; // Tamanho da barricada (AGORA UM QUADRADO)
    const float EXPLOSION_RADIUS = 150.f; // Raio máximo da explosão
    const float EXPLOSION_EXPAND_SPEED = 500.f; // Velocidade de expansão da explosão
    const float EXPLOSION_FADE_SPEED = 200.f; // Velocidade de desvanecimento da explosão

    sf::RenderWindow window(sf::VideoMode(WINDOW_W, WINDOW_H), "SFML Zomboid");
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
        return -1; 
    }

    // --- Textos do Main Menu ---
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
    sf::FloatRect goTextRect = gameOverText.getLocalBounds();
    gameOverText.setOrigin(goTextRect.left + goTextRect.width / 2.0f, goTextRect.top + goTextRect.height / 2.0f);
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

    // Texto para a tela de Pause
    sf::Text pausedText;
    pausedText.setFont(font);
    pausedText.setCharacterSize(60);
    pausedText.setFillColor(sf::Color::White);
    pausedText.setString("PAUSADO");
    sf::FloatRect pausedTextRect = pausedText.getLocalBounds();
    pausedText.setOrigin(pausedTextRect.left + pausedTextRect.width / 2.0f, pausedTextRect.top + pausedTextRect.height / 2.0f);

    sf::Text continueText;
    continueText.setFont(font);
    continueText.setCharacterSize(30);
    continueText.setFillColor(sf::Color::Cyan);
    continueText.setString("Continuar (P/Esc)");
    sf::FloatRect continueTextRect = continueText.getLocalBounds();
    continueText.setOrigin(continueTextRect.left + continueTextRect.width / 2.0f, continueTextRect.top + continueTextRect.height / 2.0f);

    sf::Text restartText;
    restartText.setFont(font);
    restartText.setCharacterSize(30);
    restartText.setFillColor(sf::Color::Cyan);
    restartText.setString("Reiniciar (R)");
    sf::FloatRect restartTextRect = restartText.getLocalBounds();
    restartText.setOrigin(restartTextRect.left + restartTextRect.width / 2.0f, restartTextRect.top + restartTextRect.height / 2.0f);

    sf::Text exitToMenuText;
    exitToMenuText.setFont(font);
    exitToMenuText.setCharacterSize(30);
    exitToMenuText.setFillColor(sf::Color::Cyan);
    exitToMenuText.setString("Menu Principal (M)");
    sf::FloatRect exitToMenuTextRect = exitToMenuText.getLocalBounds();
    exitToMenuText.setOrigin(exitToMenuTextRect.left + exitToMenuTextRect.width / 2.0f, exitToMenuTextRect.top + exitToMenuTextRect.height / 2.0f);

    sf::Text exitGameText;
    exitGameText.setFont(font);
    exitGameText.setCharacterSize(30);
    exitGameText.setFillColor(sf::Color::Cyan);
    exitGameText.setString("Sair do Jogo (Q)");
    sf::FloatRect exitGameTextRect = exitGameText.getLocalBounds();
    exitGameText.setOrigin(exitGameTextRect.left + exitGameTextRect.width / 2.0f, exitGameTextRect.top + exitGameTextRect.height / 2.0f);

    // Textos para o Cooldown das Habilidades
    sf::Text p1AbilityCooldownText;
    p1AbilityCooldownText.setFont(font);
    p1AbilityCooldownText.setCharacterSize(20);
    p1AbilityCooldownText.setFillColor(sf::Color::Red);

    sf::Text p2AbilityCooldownText;
    p2AbilityCooldownText.setFont(font);
    p2AbilityCooldownText.setCharacterSize(20);
    p2AbilityCooldownText.setFillColor(sf::Color::Blue);

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

    // Estado inicial do Jogo
    GameState currentState = MainMenu;

    // Inicializa a explosão do P1
    p1Explosion.active = false;
    p1Explosion.currentRadius = 0.f;
    p1Explosion.maxRadius = EXPLOSION_RADIUS;
    p1Explosion.expandSpeed = EXPLOSION_EXPAND_SPEED;
    p1Explosion.fadeSpeed = EXPLOSION_FADE_SPEED;
    p1Explosion.damageDealt = false;
    p1Explosion.shape.setOrigin(0,0); // será setado dinamicamente
    p1Explosion.shape.setFillColor(sf::Color(255, 165, 0, 255)); // Laranja, opaco

    // Seta a posição inicial e reseta as variáveis do jogo
    resetGame(player1Alive, player2Alive, player1, player2, base, zombies, bullets, zombieSpawnClock, WORLD_W, WORLD_H);
    
    // LOOP PRINCIPAL
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::Resized) {
                sf::FloatRect visibleArea(0, 0, (float)event.size.width, (float)event.size.height);
                gameView.setSize(event.size.width, event.size.height);
            }

            // Lógica de eventos para transição de estado e habilidades
            if (event.type == sf::Event::KeyPressed) {
                if (currentState == MainMenu && event.key.code == sf::Keyboard::Enter) {
                    currentState = Playing;
                    resetGame(player1Alive, player2Alive, player1, player2, base, zombies, bullets, zombieSpawnClock, WORLD_W, WORLD_H);
                    startNextWave(); 
                    // NOVO: Reseta cooldowns e flags de habilidade ao iniciar um jogo do menu
                    p1AbilityCooldownClock.restart();
                    p2AbilityCooldownClock.restart();
                }
                
                if (currentState == GameOverScreen && event.key.code == sf::Keyboard::R) {
                    currentState = Playing;
                    resetGame(player1Alive, player2Alive, player1, player2, base, zombies, bullets, zombieSpawnClock, WORLD_W, WORLD_H);
                    startNextWave(); 
                    // NOVO: Reseta cooldowns e flags de habilidade ao reiniciar
                    p1AbilityCooldownClock.restart();
                    p2AbilityCooldownClock.restart();
                }

                // Lógica de Pause/Unpause
                if ((event.key.code == sf::Keyboard::Escape || event.key.code == sf::Keyboard::P) && currentState != GameOverScreen && currentState != MainMenu) {
                    if (currentState == Playing) {
                        currentState = Paused;
                    } else if (currentState == Paused) {
                        currentState = Playing;
                        clock.restart(); // Reseta dt para evitar salto
                    }
                }

                // Lógica dos Botões de Pause
                if (currentState == Paused) {
                    if (event.key.code == sf::Keyboard::R) {
                        currentState = Playing;
                        resetGame(player1Alive, player2Alive, player1, player2, base, zombies, bullets, zombieSpawnClock, WORLD_W, WORLD_H);
                        startNextWave();
                        // NOVO: Reseta cooldowns e flags de habilidade ao reiniciar do pause
                        p1AbilityCooldownClock.restart();
                        p2AbilityCooldownClock.restart();
                        clock.restart();
                    } else if (event.key.code == sf::Keyboard::M) {
                        currentState = MainMenu;
                        resetGame(player1Alive, player2Alive, player1, player2, base, zombies, bullets, zombieSpawnClock, WORLD_W, WORLD_H);
                        // NOVO: Reseta cooldowns e flags de habilidade ao voltar para o menu
                        p1AbilityCooldownClock.restart();
                        p2AbilityCooldownClock.restart();
                    } else if (event.key.code == sf::Keyboard::Q) { // 'Q' para Sair do Jogo
                        window.close();
                    }
                }

                // Habilidade do Player 1 (Explosão) - Tecla E
                if (currentState == Playing && player1Alive && event.key.code == sf::Keyboard::E) {
                    if (p1AbilityCooldownClock.getElapsedTime().asSeconds() >= PLAYER1_ABILITY_COOLDOWN) {
                        p1Explosion.active = true;
                        p1Explosion.position = player1.getPosition();
                        p1Explosion.currentRadius = 0.f;
                        p1Explosion.damageDealt = false; // Reinicia para novo uso
                        p1Explosion.shape.setFillColor(sf::Color(255, 165, 0, 255)); // Começa opaco
                        p1AbilityCooldownClock.restart(); // Inicia o cooldown da habilidade

                        // NOVO: Aplica dano imediatamente ao ativar a explosão
                        // Isso garante que o dano é aplicado de forma consistente
                        for (int i = zombies.size() - 1; i >= 0; --i) {
                            if (checkCircleCollision(p1Explosion.position, p1Explosion.maxRadius, // Usa maxRadius para o dano
                                                     zombies[i].sprite.getPosition(), zombies[i].radius)) {
                                zombies.erase(zombies.begin() + i);
                                zombiesRemaining--;
                            }
                        }
                        p1Explosion.damageDealt = true; // Marca que o dano foi tratado
                    }
                }

                // Habilidade do Player 2 (Barricada) - Tecla NUM1
                if (currentState == Playing && player2Alive && event.key.code == sf::Keyboard::Numpad1) {
                    if (p2AbilityCooldownClock.getElapsedTime().asSeconds() >= PLAYER2_ABILITY_COOLDOWN) {
                        Barricade newBarricade;
                        newBarricade.shape.setSize(BARRICADE_SIZE);
                        newBarricade.shape.setFillColor(sf::Color(0, 150, 255)); // Azul padrão
                        newBarricade.shape.setOrigin(BARRICADE_SIZE.x / 2.f, BARRICADE_SIZE.y / 2.f); // Ajustado para centro do QUADRADO
                        
                        // Posição da barricada à frente do player 2
                        // Usando o raio do player + metade da largura da barricada + um pequeno espaçamento
                        float offset = player2.getRadius() + BARRICADE_SIZE.x / 2.f + 5.f; // Ajustado para quadrado
                        sf::Vector2f barricadePos = player2.getPosition() + lastDir2 * offset;
                        newBarricade.shape.setPosition(barricadePos);
                        newBarricade.health = BARRIER_LIFE; // Atribui vida inicial
                        newBarricade.maxHealth = BARRIER_LIFE; // Define vida máxima

                        // Configura o texto de vida
                        newBarricade.healthText.setFont(font);
                        newBarricade.healthText.setCharacterSize(14);
                        newBarricade.healthText.setFillColor(sf::Color::White);
                        newBarricade.healthText.setOrigin(newBarricade.healthText.getLocalBounds().width / 2.f, newBarricade.healthText.getLocalBounds().height / 2.f);
                        
                        barricades.push_back(newBarricade);
                        p2AbilityCooldownClock.restart(); // Inicia o cooldown da habilidade
                    }
                }
            }
        }

        // NOVO: Somente atualiza a lógica do jogo se não estiver pausado
        if (currentState == Playing) {
            float dt = clock.restart().asSeconds();

            // NOVO: Lógica da Habilidade do Player 1 (Explosão)
            if (p1Explosion.active) {
                // Expande o raio
                p1Explosion.currentRadius += p1Explosion.expandSpeed * dt;
                if (p1Explosion.currentRadius > p1Explosion.maxRadius) {
                    p1Explosion.currentRadius = p1Explosion.maxRadius;
                }

                // Desvanece a cor
                sf::Color currentColor = p1Explosion.shape.getFillColor();
                int alpha = currentColor.a - static_cast<int>(p1Explosion.fadeSpeed * dt);
                if (alpha < 0) alpha = 0;
                p1Explosion.shape.setFillColor(sf::Color(currentColor.r, currentColor.g, currentColor.b, alpha));

                // Atualiza o shape da explosão
                p1Explosion.shape.setRadius(p1Explosion.currentRadius);
                p1Explosion.shape.setOrigin(p1Explosion.currentRadius, p1Explosion.currentRadius); // Centraliza a explosão
                p1Explosion.shape.setPosition(p1Explosion.position);

                // Desativa a explosão quando ela se torna totalmente transparente
                if (alpha <= 0) {
                    p1Explosion.active = false;
                    p1Explosion.damageDealt = false; // Reset para próximo uso
                }
            }


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
            // window.setView(gameView); // Será aplicado na renderização


            // LÓGICA DO JOGO (MOVIMENTO, SPAWN, COLISÕES)
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

            // Movimento dos Zumbis (IA marcha para a base ou barricada)
            for (auto& z : zombies) {
                sf::Vector2f zPos = z.sprite.getPosition(); 
                sf::Vector2f currentTarget = base.getPosition(); // Target padrão é a base
                sf::Vector2f zombieDir; 

                // Verifica se há barricadas e se o zumbi deve ir para uma
                Barricade* closestBarricade = nullptr;
                float minDistanceToTarget = std::numeric_limits<float>::max(); // Distância para o alvo atual (base ou barricada)

                // Primeiro, verifique se o zumbi já está colidindo com uma barricada
                bool isCollidingWithBarricade = false;
                for (auto& bar : barricades) {
                    if (checkCircleRectCollision(zPos, z.radius, bar.shape.getPosition(), bar.shape.getSize())) {
                        closestBarricade = &bar;
                        isCollidingWithBarricade = true;
                        currentTarget = bar.shape.getPosition(); // Define a barricada como alvo para cálculo de direção
                        break; 
                    }
                }

                // Se não está colidindo, procure a barricada mais próxima no caminho para a base
                if (!isCollidingWithBarricade) {
                    for (auto& bar : barricades) {
                        sf::Vector2f barCenter = bar.shape.getPosition();
                        float distZtoBar = std::hypot(zPos.x - barCenter.x, zPos.y - barCenter.y);

                        sf::Vector2f zToBase = base.getPosition() - zPos;
                        sf::Vector2f zToBar = barCenter - zPos;
                        
                        float magnitudeZToBase = std::hypot(zToBase.x, zToBase.y);
                        float magnitudeZToBar = std::hypot(zToBar.x, zToBar.y);

                        if (magnitudeZToBase > 0 && magnitudeZToBar > 0) {
                            float dotProduct = zToBase.x * zToBar.x + zToBase.y * zToBar.y;
                            float angleCosine = dotProduct / (magnitudeZToBase * magnitudeZToBar);
                            
                            // Se a barricada estiver razoavelmente no caminho da base E estiver mais próxima que qualquer outra barricada
                            if (angleCosine > 0.7f && distZtoBar < minDistanceToTarget) { // 0.7f para um cone de visão razoável
                                minDistanceToTarget = distZtoBar;
                                closestBarricade = &bar;
                            }
                        }
                    }
                }

                if (closestBarricade) {
                    currentTarget = closestBarricade->shape.getPosition();
                }


                // Move o zumbi para o alvo (base ou barricada)
                if (currentTarget != zPos) {
                    zombieDir = currentTarget - zPos; 
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

            // NOVO: Colisão Zumbis vs Barricadas (e empurrar para trás)
            for (int i = zombies.size() - 1; i >= 0; --i) {
                for (int j = barricades.size() - 1; j >= 0; --j) {
                    // Se o zumbi está colidindo com a barricada
                    if (checkCircleRectCollision(zombies[i].sprite.getPosition(), zombies[i].radius,
                                                 barricades[j].shape.getPosition(), 
                                                 barricades[j].shape.getSize())) {
                        
                        barricades[j].health--; // Barricada perde vida
                        
                        // Empurra o zumbi para trás (oposto à direção da barricada para o zumbi)
                        sf::Vector2f zPos = zombies[i].sprite.getPosition();
                        sf::Vector2f barPos = barricades[j].shape.getPosition();
                        sf::Vector2f pushDir = zPos - barPos;
                        float len = std::hypot(pushDir.x, pushDir.y);
                        if (len > 0) pushDir /= len;
                        
                        zombies[i].sprite.move(pushDir * ZOMBIE_SPEED * dt * 2.0f); // Empurra com força
                        
                        // Remove a barricada se a vida acabar
                        if (barricades[j].health <= 0) {
                            barricades.erase(barricades.begin() + j);
                        }
                        break; // Zumbi só interage com uma barricada por vez
                    }
                }
            }

            // Colisão Zumbis vs Base (GAME OVER)
            float baseRadius = base.getSize().x / 2.f; 
            for (auto& z : zombies) {
                if (checkCircleCollision(z.sprite.getPosition(), z.radius, 
                                         base.getPosition(), baseRadius)) {
                    currentState = GameOverScreen; 
                    zombiesRemaining--; 
                    break; 
                }
            }

            // Colisão Zumbi vs Players
            if (currentState == Playing) { 
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

            // NOVO: Atualiza textos de cooldown das habilidades
            std::stringstream ssP1Ability;
            float p1RemainingCooldown = PLAYER1_ABILITY_COOLDOWN - p1AbilityCooldownClock.getElapsedTime().asSeconds();
            if (p1RemainingCooldown <= 0) {
                ssP1Ability << "P1 Habilidade: PRONTA (E)";
                p1AbilityCooldownText.setFillColor(sf::Color::Green);
            } else {
                ssP1Ability << "P1 Habilidade: " << static_cast<int>(std::ceil(p1RemainingCooldown)) << "s (E)";
                p1AbilityCooldownText.setFillColor(sf::Color::Red);
            }
            p1AbilityCooldownText.setString(ssP1Ability.str());
            p1AbilityCooldownText.setOrigin(0, p1AbilityCooldownText.getLocalBounds().height / 2.f); // Canto inferior esquerdo, alinhado à esquerda
            p1AbilityCooldownText.setPosition(viewCenter.x - gameView.getSize().x / 2.f + 10.f, viewCenter.y + gameView.getSize().y / 2.f - 40.f);

            std::stringstream ssP2Ability;
            float p2RemainingCooldown = PLAYER2_ABILITY_COOLDOWN - p2AbilityCooldownClock.getElapsedTime().asSeconds();
            if (p2RemainingCooldown <= 0) {
                ssP2Ability << "P2 Habilidade: PRONTA (L)";
                p2AbilityCooldownText.setFillColor(sf::Color::Green);
            } else {
                ssP2Ability << "P2 Habilidade: " << static_cast<int>(std::ceil(p2RemainingCooldown)) << "s (NUM1)";
                p2AbilityCooldownText.setFillColor(sf::Color::Blue);
            }
            p2AbilityCooldownText.setString(ssP2Ability.str());
            p2AbilityCooldownText.setOrigin(p2AbilityCooldownText.getLocalBounds().width, p2AbilityCooldownText.getLocalBounds().height / 2.f); // Canto inferior direito, alinhado à direita
            p2AbilityCooldownText.setPosition(viewCenter.x + gameView.getSize().x / 2.f - 10.f, viewCenter.y + gameView.getSize().y / 2.f - 40.f);

            // NOVO: Atualiza o texto e a cor da barricada
            for (auto& bar : barricades) {
                std::stringstream ssBarHealth;
                ssBarHealth << bar.health << "/" << bar.maxHealth;
                bar.healthText.setString(ssBarHealth.str());
                // Posição do texto acima da barricada
                bar.healthText.setPosition(bar.shape.getPosition().x, bar.shape.getPosition().y - BARRICADE_SIZE.y / 2.f - 10.f);
                bar.healthText.setOrigin(bar.healthText.getLocalBounds().width / 2.f, bar.healthText.getLocalBounds().height / 2.f);

                // Altera a cor da barricada de azul para vermelho conforme perde vida
                float healthRatio = static_cast<float>(bar.health) / bar.maxHealth;
                sf::Uint8 red = static_cast<sf::Uint8>(255 * (1.f - healthRatio)); // Aumenta o vermelho conforme a vida diminui
                sf::Uint8 blue = static_cast<sf::Uint8>(255 * healthRatio);        // Diminui o azul conforme a vida diminui
                bar.shape.setFillColor(sf::Color(red, 150, blue)); 
            }
        } else { // Se o jogo estiver pausado, o delta time deve ser 0 para não atualizar nada
             clock.restart();
        }
        
        // RENDERIZAÇÃO (fora do if(Playing) para que o pause mostre o estado atual)
        window.clear(sf::Color(20, 20, 20)); 

        switch (currentState) {
            case MainMenu:
                window.setView(window.getDefaultView());
                window.clear(sf::Color(10, 10, 30));
                window.draw(titleText);
                window.draw(playButtonText);
                break;

            case Playing:
            case Paused: // Ambos os estados usam a mesma lógica de renderização do jogo principal
                window.setView(gameView); // Aplica a view do jogo para ambos
                window.draw(background); 
                window.draw(base); 
                if (player1Alive) window.draw(player1);
                if (player2Alive) window.draw(player2);
                for (auto& z : zombies) window.draw(z.sprite); 
                for (auto& b : bullets) window.draw(b.shape);
                for (auto& bar : barricades) { 
                    window.draw(bar.shape);
                    window.draw(bar.healthText);
                }
                if (p1Explosion.active) window.draw(p1Explosion.shape); 
                
                // Textos do HUD (wave, zumbis, cooldowns) devem ser desenhados na view do jogo
                window.draw(waveText);
                window.draw(zombiesRemainingText);
                window.draw(p1AbilityCooldownText);
                window.draw(p2AbilityCooldownText);

                // Se estiver pausado, desenha o overlay e o menu de pause *por cima* da view do jogo
                if (currentState == Paused) {
                    window.setView(window.getDefaultView()); // Volta para a view padrão para o menu de pause
                    sf::RectangleShape darkOverlay(sf::Vector2f(WINDOW_W, WINDOW_H));
                    darkOverlay.setFillColor(sf::Color(0, 0, 0, 180));
                    window.draw(darkOverlay);

                    pausedText.setPosition(WINDOW_W / 2.f, WINDOW_H / 2.f - 100.f);
                    window.draw(pausedText);
                    continueText.setPosition(WINDOW_W / 2.f, WINDOW_H / 2.f + 0.f);
                    window.draw(continueText);
                    restartText.setPosition(WINDOW_W / 2.f, WINDOW_H / 2.f + 40.f);
                    window.draw(restartText);
                    exitToMenuText.setPosition(WINDOW_W / 2.f, WINDOW_H / 2.f + 80.f);
                    window.draw(exitToMenuText);
                    exitGameText.setPosition(WINDOW_W / 2.f, WINDOW_H / 2.f + 120.f);
                    window.draw(exitGameText);
                }
                break;

            case GameOverScreen:
                window.setView(window.getDefaultView());
                window.clear(sf::Color(30, 0, 0));
                window.draw(gameOverText);
                break;
        }
        
        // Exibe o frame final para todos os estados
        window.display();
    }

    return 0;
}
