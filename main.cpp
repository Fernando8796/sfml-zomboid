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

// NOVO: Estrutura do Zumbi (agora com sf::Sprite e float radius)
struct Zombie {
    sf::Sprite sprite; // Alterado de shape para sprite para clareza
    float radius;      // Raio para colisões circulares
};

// NOVO: Função para verificar colisão entre dois círculos
bool checkCircleCollision(sf::Vector2f p1, float r1, sf::Vector2f p2, float r2) {
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;
    float distanceSquared = dx * dx + dy * dy;
    float radiusSum = r1 + r2;
    return distanceSquared < (radiusSum * radiusSum);
}

// NOVO: Função para reiniciar o jogo (ajustada para o mundo e base)
void resetGame(bool& p1Alive, bool& p2Alive, bool& gameOver,
               sf::CircleShape& player1, sf::CircleShape& player2,
               sf::RectangleShape& base, // Adicionado a base
               std::vector<Zombie>& zombies, std::vector<Bullet>& bullets,
               sf::Clock& spawnClock, unsigned int worldW, unsigned int worldH) 
{
    p1Alive = true;
    p2Alive = true;
    gameOver = false;
    
    player1.setPosition(worldW / 3.0f, worldH / 2.0f);
    player2.setPosition(2 * worldW / 3.0f, worldH / 2.0f);
    base.setPosition(worldW / 2.0f, worldH / 2.0f); // Posiciona a base no centro do mundo
    
    zombies.clear();
    bullets.clear();
    spawnClock.restart();
}


int main() {
    srand(static_cast<unsigned>(time(0)));

    // Configurações da Janela (visível) e Mundo (tamanho do mapa)
    const unsigned int WINDOW_W = 800; // Largura da janela visível
    const unsigned int WINDOW_H = 600; // Altura da janela visível
    const unsigned int WORLD_W = 1600; // Largura do mundo do jogo
    const unsigned int WORLD_H = 1200; // Altura do mundo do jogo

    // Suas constantes personalizadas
    const float SPEED = 300.0f;
    const float BULLET_SPEED = 4800.0f; 
    const float ZOMBIE_SPEED = 60.0f; 
    const float ZOMBIE_SPAWN_TIME = 0.005f; 
    const float BALL_RADIUS = 90.f; 
    const float BULLET_RATE = 10; 

    sf::RenderWindow window(sf::VideoMode(WINDOW_W, WINDOW_H), "Defenda a Base!");
    window.setFramerateLimit(144);

    // NOVO: Configura a câmera (View)
    sf::View gameView(sf::FloatRect(0, 0, (float)WINDOW_W, (float)WINDOW_H));
    // gameView.setCenter() será atualizado no loop para seguir o jogador(es)
    window.setView(gameView); // Ativa esta view para a janela

    // NOVO: Retângulo de fundo para o mapa
    sf::RectangleShape background(sf::Vector2f((float)WORLD_W, (float)WORLD_H));
    background.setFillColor(sf::Color(40, 40, 40)); // Um cinza escuro para o mapa

    // Configuração da Base Central
    sf::RectangleShape base(sf::Vector2f(24.f, 24.f)); // Mesmo tamanho do player
    base.setFillColor(sf::Color::Yellow);
    base.setOrigin(12.f, 12.f);
    base.setPosition(WORLD_W / 2.0f, WORLD_H / 2.0f); // Posiciona a base no centro do mundo

    // Configuração de Texto de Game Over
    sf::Font font;
    // IMPORTANTE: Coloque um arquivo .ttf ou .otf (ex: zombie.otf) na pasta do executável!
    if (!font.loadFromFile("zombie.otf")) { // Usando seu nome de fonte
        // Se não encontrar, o jogo roda, mas sem texto.
    }
    sf::Text gameOverText;
    gameOverText.setFont(font);
    gameOverText.setCharacterSize(50);
    gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setString("GAME OVER!\nPressione 'R' para reiniciar.");
    sf::FloatRect textRect = gameOverText.getLocalBounds();
    gameOverText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
    gameOverText.setPosition(WINDOW_W / 2.0f, WINDOW_H / 2.0f); // Posiciona no centro da JANELA

    // NOVO: Carrega a textura do Zumbi
    sf::Texture zombieTexture;
    if (!zombieTexture.loadFromFile("zombie.png")) {
        // Tratar erro, caso não encontre o arquivo
        return -1; 
    }

    // Player 1
    sf::CircleShape player1(12.0f);
    player1.setFillColor(sf::Color::Red); 
    player1.setOrigin(player1.getRadius(), player1.getRadius());
    
    // Player 2
    sf::CircleShape player2(12.0f);
    player2.setFillColor(sf::Color::Blue);
    player2.setOrigin(player2.getRadius(), player2.getRadius());
    
    // Vetores de Balas e Zumbis
    std::vector<Bullet> bullets;
    std::vector<Zombie> zombies;
    
    // Clocks
    sf::Clock clock;
    sf::Clock shootClock1;
    sf::Clock shootClock2;
    sf::Clock zombieSpawnClock; 

    // Vetores para armazenar a última direção de movimento
    sf::Vector2f lastDir1 = {0.f, -1.f};
    sf::Vector2f lastDir2 = {0.f, -1.f};

    // Estados do Jogo
    bool player1Alive = true;
    bool player2Alive = true;
    bool gameOver = false;

    // Seta a posição inicial e reinicia o jogo
    // Passando WORLD_W e WORLD_H
    resetGame(player1Alive, player2Alive, gameOver, player1, player2, base, zombies, bullets, zombieSpawnClock, WORLD_W, WORLD_H);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            // NOVO: Lógica para redimensionamento da janela (opcional)
            if (event.type == sf::Event::Resized) {
                sf::FloatRect visibleArea(0, 0, (float)event.size.width, (float)event.size.height);
                gameView.setSize(event.size.width, event.size.height);
                window.setView(gameView); 
            }
        }

        float dt = clock.restart().asSeconds();

        // --- LÓGICA DE GAME OVER E REINÍCIO ---
        if (gameOver) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::R)) {
                // Passando WORLD_W e WORLD_H
                resetGame(player1Alive, player2Alive, gameOver, player1, player2, base, zombies, bullets, zombieSpawnClock, WORLD_W, WORLD_H);
            }
            
            // Renderiza a tela de Game Over
            window.clear(sf::Color(30, 0, 0));
            window.setView(window.getDefaultView()); // Usa a view padrão para o texto de Game Over (fixo na janela)
            window.draw(gameOverText);
            window.display();
            continue; // Pula o resto do loop do jogo
        }

        // --- LÓGICA DA CÂMERA (VIEW) ---
        sf::Vector2f viewCenter;
        if (player1Alive && player2Alive) {
            // Média entre os dois jogadores
            viewCenter.x = (player1.getPosition().x + player2.getPosition().x) / 2.f;
            viewCenter.y = (player1.getPosition().y + player2.getPosition().y) / 2.f;
        } else if (player1Alive) {
            viewCenter = player1.getPosition();
        } else if (player2Alive) {
            viewCenter = player2.getPosition();
        } else {
            // Se ambos mortos, a câmera permanece no último lugar ou no centro da base
            viewCenter = base.getPosition(); 
        }

        // Limita a câmera para não sair das bordas do mundo
        float halfViewW = gameView.getSize().x / 2.f;
        float halfViewH = gameView.getSize().y / 2.f;
        viewCenter.x = std::clamp(viewCenter.x, halfViewW, (float)WORLD_W - halfViewW);
        viewCenter.y = std::clamp(viewCenter.y, halfViewH, (float)WORLD_H - halfViewH);
        
        gameView.setCenter(viewCenter);
        window.setView(gameView); // Aplica a view atualizada

        // --- LÓGICA DO JOGO ---

        // Spawn de Zumbis
        if (zombieSpawnClock.getElapsedTime().asSeconds() > ZOMBIE_SPAWN_TIME) {
            Zombie newZombie;

            newZombie.sprite.setTexture(zombieTexture);
            
            float targetSize = 24.f; // O diâmetro do círculo que o zumbi representa (como o player)
            float originalSize = 64.f; // O tamanho da sua imagem de zumbi
            float scale = targetSize / originalSize; 
            
            newZombie.sprite.setScale(scale, scale);
            newZombie.sprite.setOrigin(originalSize / 2.f, originalSize / 2.f); // Centro da imagem original
            newZombie.radius = targetSize / 2.f; // Define o raio do zumbi para colisões
            
            // Posição aleatória na borda (fora da tela do mundo)
            int side = rand() % 4;
            float spawnX = 0, spawnY = 0;
            // Spawna um pouco mais longe para não aparecer abruptamente na view
            float spawnMargin = 60.f; 

            switch (side) {
                case 0: // Topo
                    spawnX = static_cast<float>(rand() % WORLD_W); 
                    spawnY = -spawnMargin; 
                    break;
                case 1: // Fundo
                    spawnX = static_cast<float>(rand() % WORLD_W); 
                    spawnY = static_cast<float>(WORLD_H) + spawnMargin; 
                    break;
                case 2: // Esquerda
                    spawnX = -spawnMargin; 
                    spawnY = static_cast<float>(rand() % WORLD_H); 
                    break;
                case 3: // Direita
                    spawnX = static_cast<float>(WORLD_W) + spawnMargin; 
                    spawnY = static_cast<float>(rand() % WORLD_H); 
                    break;
            }
            newZombie.sprite.setPosition(spawnX, spawnY);
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
            pos1.x = std::clamp(pos1.x, r1, (float)WORLD_W - r1); // Limites no mundo
            pos1.y = std::clamp(pos1.y, r1, (float)WORLD_H - r1); // Limites no mundo
            player1.setPosition(pos1);

            // Player 1 shooting (F)
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) {
                if (shootClock1.getElapsedTime().asMilliseconds() > BULLET_RATE) {
                    Bullet b;
                    b.shape.setRadius(BALL_RADIUS); // Usando sua constante
                    b.shape.setFillColor(player1.getFillColor());
                    b.shape.setOrigin(BALL_RADIUS / 2.f, BALL_RADIUS / 2.f); // Ajusta origem para o centro do círculo
                    b.shape.setPosition(player1.getPosition());
                    b.velocity = lastDir1 * BULLET_SPEED; // Usando sua constante
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
            pos2.x = std::clamp(pos2.x, r2, (float)WORLD_W - r2); // Limites no mundo
            pos2.y = std::clamp(pos2.y, r2, (float)WORLD_H - r2); // Limites no mundo
            player2.setPosition(pos2);

            // Player 2 shooting (Numpad0)
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad0)) {
                if (shootClock2.getElapsedTime().asMilliseconds() > BULLET_RATE) {
                    Bullet b;
                    b.shape.setRadius(BALL_RADIUS); // Usando sua constante
                    b.shape.setFillColor(player2.getFillColor());
                    b.shape.setOrigin(BALL_RADIUS / 2.f, BALL_RADIUS / 2.f); // Ajusta origem para o centro do círculo
                    b.shape.setPosition(player2.getPosition());
                    b.velocity = lastDir2 * BULLET_SPEED; // Usando sua constante
                    bullets.push_back(b);
                    shootClock2.restart();
                }
            }
        }

        // Movimento dos Zumbis (IA marcha para a base)
        sf::Vector2f targetPos = base.getPosition();
        for (auto& z : zombies) {
            sf::Vector2f zPos = z.sprite.getPosition(); // Usando z.sprite
            if (targetPos != zPos) {
                sf::Vector2f zombieDir = targetPos - zPos;
                float len = std::hypot(zombieDir.x, zombieDir.y);
                if (len > 0) zombieDir /= len;
                z.sprite.move(zombieDir * ZOMBIE_SPEED * dt); // Usando z.sprite
            }
        }

        // Atualiza balas
        for (auto& b : bullets)
            b.shape.move(b.velocity * dt);

        // Remove balas fora da tela (do MUNDO)
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [&](const Bullet& b) {
            auto p = b.shape.getPosition();
            // Remove se sair de uma margem maior fora do mundo
            return (p.y < -100 || p.y > WORLD_H + 100 || p.x < -100 || p.x > WORLD_W + 100); 
        }), bullets.end());

        // --- COLISÕES ---

        // Colisão Balas vs Zumbis (usando checkCircleCollision)
        // Itera ao contrário para evitar problemas ao apagar elementos
        for (int i = bullets.size() - 1; i >= 0; --i) {
            for (int j = zombies.size() - 1; j >= 0; --j) {
                // Balas ainda são sf::CircleShape, então usamos getRadius()
                // Zumbis agora usam o novo 'radius' da estrutura
                if (checkCircleCollision(bullets[i].shape.getPosition(), bullets[i].shape.getRadius(),
                                         zombies[j].sprite.getPosition(), zombies[j].radius)) {
                    bullets.erase(bullets.begin() + i);
                    zombies.erase(zombies.begin() + j);
                    break; // Bala destruída, sai do loop interno para esta bala
                }
            }
        }

        // Colisão Zumbis vs Base (GAME OVER - usando checkCircleCollision)
        // A base é um sf::RectangleShape, mas definimos seu tamanho para ser 24x24 (diâmetro 24, raio 12)
        float baseRadius = base.getSize().x / 2.f; // Raio da base (12.f)
        for (auto& z : zombies) {
            if (checkCircleCollision(z.sprite.getPosition(), z.radius, // Usando z.sprite e z.radius
                                     base.getPosition(), baseRadius)) {
                gameOver = true;
                break; // Fim de jogo
            }
        }

        // Colisão Zumbi vs Players (usando checkCircleCollision)
        if (!gameOver) { 
            for (auto& z : zombies) {
                if (player1Alive && checkCircleCollision(z.sprite.getPosition(), z.radius, // Usando z.sprite e z.radius
                                                         player1.getPosition(), player1.getRadius())) {
                    player1Alive = false;
                }
                if (player2Alive && checkCircleCollision(z.sprite.getPosition(), z.radius, // Usando z.sprite e z.radius
                                                         player2.getPosition(), player2.getRadius())) {
                    player2Alive = false;
                }
            }
        }

        // --- RENDERIZAÇÃO ---
        window.clear(sf::Color(20, 20, 20)); // Limpa a tela com uma cor base

        window.draw(background); // Desenha o fundo do mapa

        window.draw(base); // Desenha a base
        if (player1Alive) window.draw(player1);
        if (player2Alive) window.draw(player2);

        // Desenha todos os zumbis
        for (auto& z : zombies) window.draw(z.sprite); // Usando z.sprite
        // Desenha todas as balas
        for (auto& b : bullets) window.draw(b.shape);
        
        window.display();
    }

    return 0;
}
