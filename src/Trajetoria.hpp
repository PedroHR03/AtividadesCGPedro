#pragma once

#include <vector>
#include <glm/glm.hpp>

/* Trajetória cíclica interpolada por Catmull-Rom.
 *
 * Como usar:
 *   Trajetoria t;
 *   t.adicionarPonto({1,0,0});
 *   t.adicionarPonto({0,0,1});
 *   ...
 *   // A cada frame:
 *   t.atualizar(deltaTime);
 *   glm::vec3 pos = t.getPosicaoAtual();
 */
class Trajetoria
{
public:
    float velocidade = 1.0f; // unidades por segundo ao longo da curva

    void adicionarPonto(glm::vec3 p)
    {
        pontos.push_back(p);
    }

    void removerUltimoPonto()
    {
        if (!pontos.empty()) pontos.pop_back();
    }

    int numPontos() const { return (int)pontos.size(); }

    // Avança o parâmetro t ao longo da curva; deve ser chamado a cada frame
    void atualizar(float deltaTime)
    {
        if (pontos.size() < 2) return;

        // t global: parte inteira = índice do segmento, parte fracionária = progresso no segmento
        tGlobal += velocidade * deltaTime;

        int n = (int)pontos.size();
        // Normaliza para [0, n) de forma cíclica
        while (tGlobal >= n) tGlobal -= n;
        while (tGlobal <  0) tGlobal += n;
    }

    glm::vec3 getPosicaoAtual() const
    {
        int n = (int)pontos.size();
        if (n == 0) return glm::vec3(0.0f);
        if (n == 1) return pontos[0];

        int   i0 = (int)tGlobal;
        float t  = tGlobal - i0;

        // Índices com wrap cíclico
        int p0 = ((i0 - 1) % n + n) % n;
        int p1 = i0 % n;
        int p2 = (i0 + 1) % n;
        int p3 = (i0 + 2) % n;

        return catmullRom(pontos[p0], pontos[p1], pontos[p2], pontos[p3], t);
    }

    const std::vector<glm::vec3>& getPontos() const { return pontos; }

private:
    std::vector<glm::vec3> pontos;
    float tGlobal = 0.0f;

    // Fórmula de Catmull-Rom (alpha = 0.5 para centripetal)
    static glm::vec3 catmullRom(glm::vec3 p0, glm::vec3 p1,
                                glm::vec3 p2, glm::vec3 p3, float t)
    {
        float t2 = t * t;
        float t3 = t2 * t;

        return 0.5f * (
            (2.0f * p1) +
            (-p0 + p2) * t +
            (2.0f*p0 - 5.0f*p1 + 4.0f*p2 - p3) * t2 +
            (-p0 + 3.0f*p1 - 3.0f*p2 + p3) * t3
        );
    }
};
