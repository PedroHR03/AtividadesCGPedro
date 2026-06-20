#pragma once

#include <vector>
#include <glm/glm.hpp>

/* Trajetória cíclica por curvas de Bézier cúbicas encadeadas.
 *
 * Segmento de fechamento automático: último ponto → primeiro ponto,
 * com tangentes calculadas a partir dos pontos de controle vizinhos
 * para garantir continuidade C1 (sem "solavanco" na junção).
 */
class Bezier
{
public:
    float velocidade = 1.0f;

    void adicionarPonto(glm::vec3 p) { pontos.push_back(p); }
    void removerUltimoPonto()        { if (!pontos.empty()) pontos.pop_back(); }
    int  numPontos()    const        { return (int)pontos.size(); }

    // Segmentos explícitos disponíveis (grupos de 4 pontos encadeados)
    int numSegmentosExplicitos() const
    {
        int n = (int)pontos.size();
        return (n >= 4) ? (n - 1) / 3 : 0;
    }

    // Total de segmentos incluindo o de fechamento (se necessário)
    int numSegmentos() const
    {
        int se = numSegmentosExplicitos();
        if (se == 0) return 0;
        // Se o último ponto já coincide com o primeiro, a curva já está fechada
        if (glm::length(pontos.back() - pontos.front()) < 0.001f)
            return se;
        return se + 1; // + segmento de fechamento automático
    }

    void atualizar(float deltaTime)
    {
        if (numSegmentos() < 1) return;
        tGlobal += velocidade * deltaTime;
        float ns = (float)numSegmentos();
        while (tGlobal >= ns) tGlobal -= ns;
        while (tGlobal <  0) tGlobal += ns;
    }

    glm::vec3 getPosicaoAtual() const
    {
        int segs = numSegmentos();
        if (segs < 1) return glm::vec3(0.0f);

        int   seg = (int)tGlobal % segs;
        float t   = tGlobal - (int)tGlobal;

        int n  = (int)pontos.size();
        int se = numSegmentosExplicitos();

        glm::vec3 p0, p1, p2, p3;

        if (seg < se) {
            // Segmento explícito normal
            int base = seg * 3;
            p0 = pontos[base];
            p1 = pontos[base + 1];
            p2 = pontos[base + 2];
            p3 = pontos[base + 3];
        } else {
            // Segmento de fechamento: último ponto → primeiro ponto
            // Tangente de saída: extrapola o último par de controle (C1 contínuo)
            p0 = pontos[n - 1];
            p1 = 2.0f * pontos[n - 1] - pontos[n - 2];
            // Tangente de chegada: extrapola o primeiro par de controle (C1 contínuo)
            p2 = 2.0f * pontos[0] - pontos[1];
            p3 = pontos[0];
        }

        return bezierCubica(p0, p1, p2, p3, t);
    }

    const std::vector<glm::vec3>& getPontos() const { return pontos; }

private:
    std::vector<glm::vec3> pontos;
    float tGlobal = 0.0f;

    static glm::vec3 bezierCubica(glm::vec3 p0, glm::vec3 p1,
                                  glm::vec3 p2, glm::vec3 p3, float t)
    {
        float u  = 1.0f - t;
        float u2 = u * u,  u3 = u2 * u;
        float t2 = t * t,  t3 = t2 * t;
        return u3*p0 + 3.0f*u2*t*p1 + 3.0f*u*t2*p2 + t3*p3;
    }
};
