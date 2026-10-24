
#include <cmath>
#include <algorithm>
#include <vector>
#include <numeric>
#include <cstdint>
#include <unordered_map>
#define STB_IMAGE_IMPLEMENTATION   // génère le code dans cette unité de compilation uniquement
#include "stb_image.h"
#include <cstdio>

struct Point {
    float x;
    float y;
    int cluster;
    int top3[150];
    int value;
    char label;
};

struct KreamPixel {
    Point* buffer ; 
} ;

extern "C" {
    __declspec(dllexport) float CalculateRadius(
        Point* dataset_ptr, int size, int k, float alpha
    ) {
        if (dataset_ptr == nullptr || size <= k || k < 1 || alpha <= 0.0f)
            return 0.0f;

        std::vector<float> nearest_distances;
        nearest_distances.reserve(size);

        for (int i = 0; i < size; i++) {
            std::vector<float> distances;
            distances.reserve(size - 1);

            for (int j = 0; j < size; j++) {
                if (i == j)
                    continue;

                float dx = dataset_ptr[i].x - dataset_ptr[j].x;
                float dy = dataset_ptr[i].y - dataset_ptr[j].y;
                distances.push_back(std::sqrt(dx * dx + dy * dy));
            }

            std::nth_element(
                distances.begin(),
                distances.begin() + (k - 1),
                distances.end()
            );
            nearest_distances.push_back(distances[k - 1]);
        }

        const int middle = size / 2;
        std::nth_element(
            nearest_distances.begin(),
            nearest_distances.begin() + middle,
            nearest_distances.end()
        );

        // Pour un nombre pair de points, prendre la moyenne des deux valeurs centrales.
        float median = nearest_distances[middle];
        if (size % 2 == 0) {
            const float lower = *std::max_element(
                nearest_distances.begin(),
                nearest_distances.begin() + middle
            );
            median = (lower + median) / 2.0f;
        }

        return median * alpha;
    }
    // La fonction renvoie la classe prédite (-1 si aucun voisin trouvé)
    __declspec(dllexport) int KreamKNN_Classify(float test_x, float test_y, Point* dataset, int dataset_size, int* out_top3) {
        
        if (dataset_size <= 0 || dataset == nullptr) {
            return -1; 
        }

        float top_dists[150];
        
        // 1. Initialisation propre des distances à l'infini et des indices à -1
        for (int i = 0; i < 150; i++) {
            top_dists[i] = 1e9;
            out_top3[i] = -1;
        }

        float search_radius = 300.0;

        // 2. Recherche et insertion dans le Top 150 trié
        for (int index = 0; index < dataset_size; index++) {
            float dx = test_x - dataset[index].x;
            float dy = test_y - dataset[index].y;

            // Filtre spatial (bounding box)
            if (std::abs(dx) > search_radius || std::abs(dy) > search_radius) {
                continue; 
            }

            float d_carre = (dx * dx) + (dy * dy);

            // Si le point est plus proche que le pire du top 150 actuel
            if (d_carre < top_dists[149]) {
                // Trouver sa position d'insertion par décalage
                int i = 148;
                while (i >= 0 && top_dists[i] > d_carre) {
                    top_dists[i + 1] = top_dists[i];
                    out_top3[i + 1] = out_top3[i];
                    i--;
                }
                top_dists[i + 1] = d_carre;
                out_top3[i + 1] = index;
            }
        }

        // 3. Vote à la majorité parmi les voisins valides du Top 150
        int unique_classes[150];
        int class_counts[150];
        int unique_count = 0;

        for (int i = 0; i < 150; i++) {
            if (out_top3[i] != -1) {
                int neighbor_cluster = dataset[out_top3[i]].cluster;
                
                bool found = false;
                for (int j = 0; j < unique_count; j++) {
                    if (unique_classes[j] == neighbor_cluster) {
                        class_counts[j]++;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    unique_classes[unique_count] = neighbor_cluster;
                    class_counts[unique_count] = 1;
                    unique_count++;
                }
            }
        }

        // S'il n'y a aucun voisin valide dans la zone
        if (unique_count == 0) {
            return -1;
        }

        // 4. Trouver la classe ayant le plus de votes
        int best_class = unique_classes[0];
        int max_votes = class_counts[0];

        for (int j = 1; j < unique_count; j++) {
            if (class_counts[j] > max_votes) {
                max_votes = class_counts[j];
                best_class = unique_classes[j];
            }
        }

        return best_class; // Retourne la classe gagnante
    }

    __declspec(dllexport) float KreamRegression(float test_x, float test_y, Point* dataset, int size_dataset, float search_radius = 300.0) {
        std::vector<int> neighboor_dist;
        std::vector<int> neighboor_value;
        
        for (int index = 0; index < size_dataset; index++) {
            if (
                (float)test_x < dataset[index].x - search_radius || (float)test_x > dataset[index].x + search_radius ||
                (float)test_y < dataset[index].y - search_radius || (float)test_y > dataset[index].y + search_radius
            ) {
                continue; 
            } else {
                float dx = (float)test_x - dataset[index].x;
                float dy = (float)test_y - dataset[index].y;
                float distance = (dx * dx) + (dy * dy);

                neighboor_dist.push_back(distance);
                neighboor_value.push_back(dataset[index].value);
            }
        }

        float sigma_neigh = 0.0;
        float sigma_one = 0.0;

        for (size_t i = 0; i < neighboor_dist.size(); i++) {
            float weight = 1.0 / (neighboor_dist[i] + 0.0001); 
            
            sigma_neigh += neighboor_value[i] * weight; 
            sigma_one += weight;                        
        }

        if (sigma_one == 0.0) {
            return 0; 
        }

        float predicted_value = sigma_neigh / sigma_one;

        return static_cast<int>(predicted_value);
    }

    __declspec(dllexport) int KreamHitler (
            int color_image, int shape_image,
            std::uint8_t hex, Point* dataset, 
            int size_dataset
    ) {
        std::vector<Point> neighboors; 
        Point test;   
        test.x = (float)color_image;
        test.y = (float)shape_image; 
        test.value = hex;
        
        for (int index = 0; index < size_dataset; index++) {
            if (std::abs(dataset[index].value - test.value) <= 300) {
                neighboors.push_back(dataset[index]);
            } 
            else continue; 
        }
        
        if (neighboors.empty()) {
            return -1;
        }

        // Déclaration de la map de comptage des votes
        std::unordered_map<int, int> class_votes;
        
        for (size_t index = 0; index < neighboors.size(); index++) {
            int current_class = neighboors[index].cluster; 
            class_votes[current_class]++;
        }
        
        // Sélection de la classe qui a le plus de votes
        int best_class = -1;
        int max_votes = -1;
        for (const auto& pair : class_votes) {
            if (pair.second > max_votes) {
                max_votes = pair.second;
                best_class = pair.first;
            }
        }
        
        return best_class;
    }
    __declspec(dllexport) const char* KreamPixel(char* image_name, char* temoin_image) {
        int w1, h1, c1;
        int w2, h2, c2;

        // 1. On charge les deux images en RGB (3 canaux)
        unsigned char* img = stbi_load(image_name, &w1, &h1, &c1, 3);
        unsigned char* temoin = stbi_load(temoin_image, &w2, &h2, &c2, 3);

        // Correction : return 0 au lieu de return vide (car la fonction renvoie un int)
        if (!img || !temoin) {
            if (img) stbi_image_free(img);
            if (temoin) stbi_image_free(temoin);
            return "Erreur de chargement"; 
        }

        // 2. unsigned int (au lieu de char) pour ne pas saturer à 255, et initialisé à 0
        unsigned int color_disp[4096] = {0}; 
        unsigned int color_temo[4096] = {0}; 

        // 3. Remplissage pour l'image Test (pas de 3 en 3 pour le buffer RGB)
        int total_bytes_img = w1 * h1 * 3;
        for (int i = 0; i < total_bytes_img; i += 3) {
            unsigned char red   = img[i]     >> 4; // Quantification 4 bits
            unsigned char green = img[i + 1] >> 4;
            unsigned char blue  = img[i + 2] >> 4; 

            // Index unique sécurisé (0 à 4095)
            int indice = (red << 8) | (green << 4) | blue;
            color_disp[indice]++;
        }

        // 4. Remplissage pour l'image Témoin (corrigé pour pointer sur 'temoin' et non 'img')
        int total_bytes_temoin = w2 * h2 * 3;
        for (int i = 0; i < total_bytes_temoin; i += 3) {
            unsigned char red   = temoin[i]     >> 4; // Quantification 4 bits
            unsigned char green = temoin[i + 1] >> 4;
            unsigned char blue  = temoin[i + 2] >> 4; 

            int indice = (red << 8) | (green << 4) | blue;
            color_temo[indice]++;
        }

        // 5. Tes sigmas (somme des histogrammes)
        int diff =  0 ; 
        for (int index = 0; index < 4096; index++) {
            if (color_disp[index] > color_temo[index]) diff += color_disp[index] - color_temo[index] ; 
            else diff += color_temo[index] - color_disp[index] - color_temo[index] ;
        } 

        int seuil = total_bytes_temoin * 0.35 ;

        // Libération de la mémoire
        stbi_image_free(img);
        stbi_image_free(temoin);

        // 6. Verdict final
        if (diff > seuil) return "YES" ; 
        else return "No" ;
    }
}