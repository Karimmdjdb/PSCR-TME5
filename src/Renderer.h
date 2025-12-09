#pragma once

#include "Scene.h"
#include "Image.h"
#include "Ray.h"
#include "Pool.h"
#include "Job.h"
#include <thread>

namespace pr {

// Classe pour rendre une scène dans une image
class Renderer {
public:
    // Rend la scène dans l'image
    void render(const Scene& scene, Image& img) {
        // les points de l'ecran, en coordonnées 3D, au sein de la Scene.
        // on tire un rayon de l'observateur vers chacun de ces points
        const Scene::screen_t& screen = scene.getScreenPoints();

        // pour chaque pixel, calculer sa couleur
        for (int x = 0; x < scene.getWidth(); x++) {
            for (int y = 0; y < scene.getHeight(); y++) {
                // le point de l'ecran par lequel passe ce rayon
                auto& screenPoint = screen[y][x];
                // le rayon a inspecter
                Ray ray(scene.getCameraPos(), screenPoint);

                int targetSphere = scene.findClosestInter(ray);

                if (targetSphere == -1) {
                    // keep background color
                    continue;
                } else {
                    const Sphere& obj = scene.getObject(targetSphere);
                    // pixel prend la couleur de l'objet
                    Color finalcolor = scene.computeColor(obj, ray);
                    // mettre a jour la couleur du pixel dans l'image finale.
                    img.pixel(x, y) = finalcolor;
                }
            }
        }
    }

    void renderThreadPerPixel(const Scene &scene, Image &img) {
        const Scene::screen_t& screen = scene.getScreenPoints();
        std::vector<std::thread> threads;
        for (int x = 0; x < scene.getWidth(); x++)
        for (int y = 0; y < scene.getHeight(); y++)
        threads.emplace_back(std::thread([&, x, y]() -> void {
            // le point de l'ecran par lequel passe ce rayon
            auto& screenPoint = screen[y][x];
            // le rayon a inspecter
            Ray ray(scene.getCameraPos(), screenPoint);

            int targetSphere = scene.findClosestInter(ray);

            if (targetSphere == -1) {
                // keep background color
                return;
            } else {
                const Sphere& obj = scene.getObject(targetSphere);
                // pixel prend la couleur de l'objet
                Color finalcolor = scene.computeColor(obj, ray);
                // mettre a jour la couleur du pixel dans l'image finale.
                img.pixel(x, y) = finalcolor;
            }
        }));

        for(auto& t : threads) t.join();
    }

    void renderThreadPerRow(const Scene &scene, Image &img) {
        const Scene::screen_t& screen = scene.getScreenPoints();
        std::vector<std::thread> threads;
        for (int x = 0; x < scene.getWidth(); x++)
        threads.emplace_back(std::thread([&, x]() -> void {
            for (int y = 0; y < scene.getHeight(); y++) {
                // le point de l'ecran par lequel passe ce rayon
                auto& screenPoint = screen[y][x];
                // le rayon a inspecter
                Ray ray(scene.getCameraPos(), screenPoint);

                int targetSphere = scene.findClosestInter(ray);

                if (targetSphere == -1) {
                    // keep background color
                    continue;
                } else {
                    const Sphere& obj = scene.getObject(targetSphere);
                    // pixel prend la couleur de l'objet
                    Color finalcolor = scene.computeColor(obj, ray);
                    // mettre a jour la couleur du pixel dans l'image finale.
                    img.pixel(x, y) = finalcolor;
                }
            }
        }));

        for(auto& t : threads) t.join();
    }

    void renderThreadManual(const Scene &scene, Image &img, const int& nbThread) {
        const int h = scene.getHeight();
        const Scene::screen_t& screen = scene.getScreenPoints();
        std::vector<int> chunks;
        chunks.push_back(0);
        for(int t=0; t<nbThread; ++t) {
            if(t==nbThread-1) chunks.push_back(chunks[t]+h/nbThread+h%nbThread);
            else chunks.push_back(chunks[t]+h/nbThread);
        }

        std::vector<std::thread> threads;
        for(int t=0; t<nbThread; ++t) {
            threads.emplace_back(std::thread([&, t] () -> void {
                for(int y=chunks[t]; y<chunks[t+1]; ++y)
                for (int x = 0; x < scene.getWidth(); x++) {
                    // le point de l'ecran par lequel passe ce rayon
                    auto& screenPoint = screen[y][x];
                    // le rayon a inspecter
                    Ray ray(scene.getCameraPos(), screenPoint);

                    int targetSphere = scene.findClosestInter(ray);

                    if (targetSphere == -1) {
                        // keep background color
                        continue;
                    } else {
                        const Sphere& obj = scene.getObject(targetSphere);
                        // pixel prend la couleur de l'objet
                        Color finalcolor = scene.computeColor(obj, ray);
                        // mettre a jour la couleur du pixel dans l'image finale.
                        img.pixel(x, y) = finalcolor;
                    }
                }

            }));
        }
        for(auto& t : threads) t.join();
    }

    void renderPoolPixel(const Scene &scene, Image &img, const int& nbThread) {
        Pool pool(100);
        pool.start(nbThread);

        for(int x=0; x<scene.getWidth(); ++x)
        for(int y=0; y<scene.getHeight(); ++y)
        pool.submit(new PixelJob(scene, img, x, y));

        pool.stop();
    }
};

} // namespace pr