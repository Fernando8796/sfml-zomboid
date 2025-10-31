#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <algorithm>
#include <limits>
#include <cstdlib> // Para números aleatórios (rand)
#include <ctime>   // Para a semente de números aleatórios (time)
#include <string>  // Para o texto de Game Over

// Estrutura da Bala
struct Bullet {
    sf::CircleShape shape;
    sf::Vector2f velocity;
};

// NOVO: Estrutura do Zumbi
struct Zombie {
    sf::CircleShape shape;
};

// NOVO: Função para reiniciar o jogo
void resetGame(bool& p1Alive, bool& p2Alive, bool& gameOver,
               sf::CircleShape& player1, sf::CircleShape& player2,
               std::vector<Zombie>& zombies, std::vector<Bullet>& bullets,
               sf::Clock& spawnClock, unsigned int w, unsigned int h) 
{
    p1Alive = true;
    p2Alive = true;
    gameOver = false;
    
    player1.setPosition(w / 3.0f, h / 2.0f);
    player2.setPosition(2 * w / 3.0f, h / 2.0f);
    
    zombies.clear();
    bullets.clear();
    spawnClock.restart();
}


int main() {
    srand(static_cast<unsigned>(time(0)));

    // Configurações da Janela e Velocidades
    const unsigned int WINDOW_W = 800;
    const unsigned int WINDOW_H = 600;
    const float SPEED = 300.0f;
    const float BULLET_SPEED = 4800.0f; // (deafult: 600.f)
    const float ZOMBIE_SPEED = 60.0f; // NOVO: Zumbis mais lentos
    const float ZOMBIE_SPAWN_TIME = 0.005f; // NOVO: Zumbis nascem rápido (default: 0.5f)
    const float BALL_RADIUS = 90.f; //NOVO: Raio da bola (deafult: 5.f)
    const float BULLET_RATE = 10; // (default: 200)

    sf::RenderWindow window(sf::VideoMode(WINDOW_W, WINDOW_H), "Defenda a Base!");
    window.setFramerateLimit(144);

    // NOVO: Configuração da Base Central
    sf::RectangleShape base(sf::Vector2f(24.f, 24.f)); // Mesmo tamanho do player
    base.setFillColor(sf::Color::Yellow);
    base.setOrigin(12.f, 12.f);
    base.setPosition(WINDOW_W / 2.0f, WINDOW_H / 2.0f);

    // NOVO: Configuração de Texto de Game Over
    sf::Font font;
    // IMPORTANTE: Coloque um arquivo .ttf (ex: arial.ttf) na pasta do executável!
    if (!font.loadFromFile("zombie.otf")) {
        // Se não encontrar, o jogo roda, mas sem texto.
    }
    sf::Text gameOverText;
    gameOverText.setFont(font);
    gameOverText.setCharacterSize(50);
    gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setString("GAME OVER!\nPressione 'R' para reiniciar.");
    sf::FloatRect textRect = gameOverText.getLocalBounds();
    gameOverText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
    gameOverText.setPosition(WINDOW_W / 2.0f, WINDOW_H / 2.0f);


    // Player 1
    sf::CircleShape player1(12.0f);
    player1.setFillColor(sf::Color::Red); // NOVO: Cor mudada para Vermelho
    player1.setOrigin(player1.getRadius(), player1.getRadius());
    
    // Player 2
    sf::CircleShape player2(12.0f);
    player2.setFillColor(sf::Color::Blue);
    player2.setOrigin(player2.getRadius(), player2.getRadius());
    
    // NOVO: Vetores de Balas e Zumbis
    std::vector<Bullet> bullets;
    std::vector<Zombie> zombies;
    
    // Clocks
    sf::Clock clock;
    sf::Clock shootClock1;
    sf::Clock shootClock2;
    sf::Clock zombieSpawnClock; // NOVO: Clock para o spawn de Zumbis

    // Vetores para armazenar a última direção de movimento
    sf::Vector2f lastDir1 = {0.f, -1.f};
    sf::Vector2f lastDir2 = {0.f, -1.f};

    // NOVO: Estados do Jogo
    bool player1Alive = true;
    bool player2Alive = true;
    bool gameOver = false;

    // Seta a posição inicial
    resetGame(player1Alive, player2Alive, gameOver, player1, player2, zombies, bullets, zombieSpawnClock, WINDOW_W, WINDOW_H);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        float dt = clock.restart().asSeconds();

        // --- LÓGICA DE GAME OVER E REINÍCIO ---
        if (gameOver) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::R)) {
                resetGame(player1Alive, player2Alive, gameOver, player1, player2, zombies, bullets, zombieSpawnClock, WINDOW_W, WINDOW_H);
            }
            
            // Renderiza a tela de Game Over
            window.clear(sf::Color(30, 0, 0));
            window.draw(gameOverText);
            window.display();
            continue; // Pula o resto do loop do jogo
        }

        // --- LÓGICA DO JOGO ---

        // NOVO: Spawn de Zumbis
        if (zombieSpawnClock.getElapsedTime().asSeconds() > ZOMBIE_SPAWN_TIME) {
            Zombie newZombie;
            newZombie.shape.setRadius(12.f); // Tamanho do player
            newZombie.shape.setFillColor(sf::Color::Green); // Cor verde
            newZombie.shape.setOrigin(12.f, 12.f);

            // Posição aleatória na borda (fora da tela)
            int side = rand() % 4;
            float spawnX = 0, spawnY = 0;
            switch (side) {
                case 0: // Topo
                    spawnX = static_cast<float>(rand() % WINDOW_W);
                    spawnY = -20.f;
                    break;
                case 1: // Fundo
                    spawnX = static_cast<float>(rand() % WINDOW_W);
                    spawnY = static_cast<float>(WINDOW_H) + 20.f;
                    break;
                case 2: // Esquerda
                    spawnX = -20.f;
                    spawnY = static_cast<float>(rand() % WINDOW_H);
                    break;
                case 3: // Direita
                    spawnX = static_cast<float>(WINDOW_W) + 20.f;
                    spawnY = static_cast<float>(rand() % WINDOW_H);
                    break;
            }
            newZombie.shape.setPosition(spawnX, spawnY);
            zombies.push_back(newZombie);
            zombieSpawnClock.restart();
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
            pos1.x = std::clamp(pos1.x, r1, (float)WINDOW_W - r1);
            pos1.y = std::clamp(pos1.y, r1, (float)WINDOW_H - r1);
            player1.setPosition(pos1);

            // Player 1 shooting (F)
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) {
                if (shootClock1.getElapsedTime().asMilliseconds() > BULLET_RATE) {
                    Bullet b;
                    b.shape.setRadius(BALL_RADIUS);
                    b.shape.setFillColor(player1.getFillColor());
                    b.shape.setOrigin(5.f, 5.f);
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
            pos2.x = std::clamp(pos2.x, r2, (float)WINDOW_W - r2);
            pos2.y = std::clamp(pos2.y, r2, (float)WINDOW_H - r2);
            player2.setPosition(pos2);

            // Player 2 shooting (Numpad0)
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad0)) {
                if (shootClock2.getElapsedTime().asMilliseconds() > BULLET_RATE) {
                    Bullet b;
                    b.shape.setRadius(BALL_RADIUS);
                    b.shape.setFillColor(player2.getFillColor());
                    b.shape.setOrigin(5.f, 5.f);
                    b.shape.setPosition(player2.getPosition());
                    b.velocity = lastDir2 * BULLET_SPEED;
                    bullets.push_back(b);
                    shootClock2.restart();
                }
            }
        }

        // NOVO: Movimento dos Zumbis (IA marcha para a base)
        sf::Vector2f targetPos = base.getPosition();
        for (auto& z : zombies) {
            sf::Vector2f zPos = z.shape.getPosition();
            if (targetPos != zPos) {
                sf::Vector2f zombieDir = targetPos - zPos;
                float len = std::hypot(zombieDir.x, zombieDir.y);
                if (len > 0) zombieDir /= len;
                z.shape.move(zombieDir * ZOMBIE_SPEED * dt);
            }
        }

        // Atualiza balas
        for (auto& b : bullets)
            b.shape.move(b.velocity * dt);

        // Remove balas fora da tela
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [&](const Bullet& b) {
            auto p = b.shape.getPosition();
            return (p.y < 0 || p.y > WINDOW_H || p.x < 0 || p.x > WINDOW_W);
        }), bullets.end());

        // --- COLISÕES ---

        // NOVO: Colisão Balas vs Zumbis
        // Itera ao contrário para evitar problemas ao apagar elementos
        for (int i = bullets.size() - 1; i >= 0; --i) {
            for (int j = zombies.size() - 1; j >= 0; --j) {
                if (bullets[i].shape.getGlobalBounds().intersects(zombies[j].shape.getGlobalBounds())) {
                    bullets.erase(bullets.begin() + i);
                    zombies.erase(zombies.begin() + j);
                    break; // Bala destruída, sai do loop interno
                }
            }
        }

        // NOVO: Colisão Zumbis vs Base (GAME OVER)
        for (auto& z : zombies) {
            if (z.shape.getGlobalBounds().intersects(base.getGlobalBounds())) {
                gameOver = true;
                break; // Fim de jogo
            }
        }

        // Colisão Zumbi vs Players
        if (!gameOver) { // Só checa se o jogo não acabou de terminar
            for (auto& z : zombies) {
                if (player1Alive && z.shape.getGlobalBounds().intersects(player1.getGlobalBounds())) {
                    player1Alive = false;
                }
                if (player2Alive && z.shape.getGlobalBounds().intersects(player2.getGlobalBounds())) {
                    player2Alive = false;
                }
            }
        }

        // --- RENDERIZAÇÃO ---
        window.clear(sf::Color(20, 20, 20));

        window.draw(base); // Desenha a base
        if (player1Alive) window.draw(player1);
        if (player2Alive) window.draw(player2);

        // Desenha todos os zumbis
        for (auto& z : zombies) window.draw(z.shape);
        // Desenha todas as balas
        for (auto& b : bullets) window.draw(b.shape);
        
        window.display();
    }

    return 0;
}
