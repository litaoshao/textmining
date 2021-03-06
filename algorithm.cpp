#include <time.h>
#include <stdlib.h>
#include <iostream>

#include "algorithm.h"
#include "count_Z.h"
#include "func.h"

double perform_algorithm(const DocsWords & docs_words,
                        PhiTheta & phi_theta,
                        Hidden_Collection & hidden_collection,
                        Prepare & prepare,
                        GetDelta & get_delta,
                        UpdateTime & update_time,
                        Stabilized & stabilized,
                        double gamma) {
    clock_t zero_time = clock();
    int iteration = 0;
    const bool echo = true;
    do {
        for (int d = 0; d < docs_words.docs_number(); ++d) {
            double & nu = hidden_collection.doc[d].nu;
            double new_nu = 0;
            int times = 0;
            if (my_abs(gamma) > 0.00001) {   // magic number
                times = 1;          // CHANGE HERE
            } else {
                times = 1;
                gamma = 0;
            }
            for (int j = 0; j < times; ++j) {
                for (int w = 0; w < docs_words.unique_words_number(d); ++w) {
                    Hidden_Word & word = hidden_collection.doc[d].word[w];
                    const int word_id = docs_words.word_id(d, w);
                    const int n_dw = docs_words.word_counter(d, w);
                    const double Z = count_Z(d, word_id, word, phi_theta);

                    for (size_t i = 0; i < word.topic.size(); ++i) {
                        std::pair<int, double> & topic =  word.topic[i];
                        prepare(d, w, topic, phi_theta);
                        double delta = get_delta(d, word_id, n_dw, i, word, phi_theta, Z);
                        delta = (delta != delta) ? 0 : delta; // silly protection from errors
                        topic.second += delta;
                        phi_theta.inc(d, word_id, topic.first, delta);
                    }
                    if (update_time(d, w, docs_words)) {
                        phi_theta.update();
                    }
                    const double Z_new = count_Z(d, word_id, word, phi_theta) - word.pi;
                    //const double Z_new = Z - word.pi;
                    double temp_pi = n_dw * gamma / nu - Z_new;
                    //const double old_pi = word.pi;
                    word.pi = my_pos((temp_pi != temp_pi) ? 0 : temp_pi); // silly protection from errors
                    new_nu += docs_words.word_counter(d, w) * word.pi / (word.pi + Z_new);

                    if (echo) {
                        double h_sum = 0;
                        for (size_t i = 0; i < word.topic.size(); ++i) {
                           h_sum += word.topic[i].second;
                        }
                        //std::cout << " Hs-n_dw=" << h_sum - docs_words.word_counter(d, w) << " "; // 1 expected
                        //std::cout << " Hs=" << h_sum << " "; // 1 expected
                    }
                }
                nu = new_nu;

                if (echo) {
                    double word_pi_sum = 0;
                    for (int ww = 0; ww < docs_words.unique_words_number(d); ++ww) {
                        word_pi_sum += hidden_collection.doc[d].word[ww].pi;
                    }
                    for (int ww = 0; ww < docs_words.unique_words_number(d); ++ww) {
                        hidden_collection.doc[d].word[ww].pi *= gamma/word_pi_sum;
                    }
                    word_pi_sum = 0;
                    for (int ww = 0; ww < docs_words.unique_words_number(d); ++ww) {
                        word_pi_sum += hidden_collection.doc[d].word[ww].pi;
                    }
                    std::cout   << " " << word_pi_sum / gamma << " " << nu
                                << " " << docs_words.unique_words_number(d)
                                << " " << docs_words.total_words_number(d)
                                << " | ";
                }

            }
            if (echo || ((d % 20) == 0)) { std::cout << "\r" << "d" << d << " of "<< docs_words.docs_number(); }
        }
        std::cout << "\r" <<"Iteration made: " << ++iteration;
    } while (!stabilized(phi_theta));
    std::cout << std::endl;
    return (float)(clock() - zero_time) / CLOCKS_PER_SEC;
}
