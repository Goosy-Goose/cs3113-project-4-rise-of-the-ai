/**
* Author: Selina Gong
* Assignment: Rise of the AI
* Date due: 2024-11-03, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/


#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
//#define LOG(argument) System::Diagnostics::Debug::WriteLine(argument);
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define ENEMY_COUNT 3 // needs to be changed
#define LEVEL_WIDTH 14
#define LEVEL_HEIGHT 7


#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_mixer.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include <cstdlib>
#include <fstream>
#include "Map.h"
#include "Entity.h"

// ––––– STRUCTS AND ENUMS ––––– //
struct GameState
{
    Entity* player;
    Entity* enemies;

    Map* map;

    Mix_Music* bgm;
    Mix_Chunk* jump_sfx;
};

// ––––– CONSTANTS ––––– //
const int WINDOW_WIDTH  = 640*1.5,
          WINDOW_HEIGHT = 480*1.5;

const float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;
const char SPRITESHEET_FILEPATH[]      = "assets/DinoSpritesSheet.png";
const char ENEMY_SPRITESHEET_FILEPATH[]= "assets/EnemySpritesSheet.png";
const char MAP_TILESHEET_FILEPATH[]    = "assets/TerrainTileSheet.png";
constexpr char FONTSHEET_FILEPATH[]    = "assets/font1.png";

constexpr int FONTBANK_SIZE = 16;
GLuint g_font_id;

const int NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL  = 0;
const GLint TEXTURE_BORDER   = 0;

const int CD_QUAL_FREQ    = 44100,
          AUDIO_CHAN_AMT  = 2,     // stereo
          AUDIO_BUFF_SIZE = 4096;

const char BGM_FILEPATH[] = "assets/Doobly_Doo.mp3",
           SFX_FILEPATH[] = "assets/player_jump.wav";

const int PLAY_ONCE = 0,    // play once, loop never
          NEXT_CHNL = -1,   // next available channel
          ALL_SFX_CHNL = -1;


Mix_Music *g_music;
Mix_Chunk *g_jump_sfx;

unsigned int LEVEL_DATA[] =
{
   42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
   42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
   42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
   42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
   42, 42, 42, 42, 42, 42, 17, 18, 18, 19, 42, 42, 42, 42,
    6,  7,  7,  7,  8, 42, 42, 42, 42, 42, 42, 42, 42, 42,
   28, 29, 29, 29, 30, 12, 13, 13, 13, 13, 13, 13, 13, 14
};

constexpr float GRAVITY = /*0.0f;*/ -7.0f;


// ––––– GLOBAL VARIABLES ––––– //
GameState g_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

//// im going to cry 
//std::ofstream log;
//log.open("log.txt");
//log << "Writing this to a file.\n";
//log.close();

// ––––– GENERAL FUNCTIONS ––––– //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    stbi_image_free(image);
    
    return textureID;
}

void draw_text(ShaderProgram* program, GLuint font_texture_id, std::string text,
    float font_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for
    // each character. Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their
        //    position relative to the whole sentence)
        int spritesheet_index = (int)text[i];  // ascii value of character
        float offset = (font_size + spacing) * i;

        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float)(spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float)(spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
            });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);

    program->set_model_matrix(model_matrix);
    glUseProgram(program->get_program_id());

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0,
        vertices.data());
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0,
        texture_coordinates.data());
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}


// ––––– GAMEPLAY LOOP FUNCTIONS ––––– //
void initialise()
{
    srand(time(0));
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("please release me, i need to use the sleep ",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    

    // ––––– VIDEO ––––– //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_program.set_projection_matrix(g_projection_matrix);
    g_program.set_view_matrix(g_view_matrix);
    
    glUseProgram(g_program.get_program_id());
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    // ––––– BGM ––––– //
    Mix_OpenAudio(CD_QUAL_FREQ, MIX_DEFAULT_FORMAT, AUDIO_CHAN_AMT, AUDIO_BUFF_SIZE);
    g_state.bgm = Mix_LoadMUS(BGM_FILEPATH);
    //if (!g_state.bgm) { assert(false); }
    Mix_PlayMusic(g_state.bgm, -1); // -1 to loop forever

    // ––––– SFX ––––– //
    g_state.jump_sfx = Mix_LoadWAV(SFX_FILEPATH);

    // ––––– FONT ––––– //
    g_font_id = load_texture(FONTSHEET_FILEPATH);
    
    // ––––– MAP ––––– //
    GLuint map_texture_id = load_texture(MAP_TILESHEET_FILEPATH);
    g_state.map = new Map(LEVEL_WIDTH, LEVEL_HEIGHT, LEVEL_DATA, map_texture_id, 1.0f, 22, 11);

    // ––––– PLAYER ––––– //

    GLuint player_texture_id = load_texture(SPRITESHEET_FILEPATH);

    std::vector<std::vector<int>> player_animations = {
       {0, 1, 2, 3},              // IDLE animation frames
       {6, 7, 8, 9, 10, 11},      // MOVE_RIGHT animation frames
       {12, 13, 14, 15, 16, 17},  // MOVE_LEFT animation frames
       {19}           // JUMP animation frames
    };

    glm::vec3 acceleration = glm::vec3(0.0f, GRAVITY, 0.0f);

    g_state.player = new Entity(
        player_texture_id,         // texture id
        3.0f,                      // speed
        acceleration,              // acceleration
        5.0f,                      // jumping power
        player_animations,         // animation index sets
        IDLE,
        0.0f,                      // animation time
        4,                         // animation frame amount
        0,                         // current animation index
        6,                         // animation column amount
        4,                         // animation row amount
        0.8333f,                   // width (15/18)
        1.0f,                      // height (18/18)
        PLAYER                     // entity type
    );

    g_state.player->set_position(glm::vec3(0.0f, 5.5f, 0));

    // ––––– ENEMIES ––––– //
    GLuint enemy_texture_id = load_texture(ENEMY_SPRITESHEET_FILEPATH);
    std::vector<std::vector<int>> enemy_animations = {
       {0, 1, 2, 3},              // AI_IDLE animation frames
       {6, 7, 8, 9, 10, 11},      // AI_RIGHT animation frames
       {12, 13, 14, 15, 16, 17},  // AI_LEFT animation frames
       {0},                       // AI_JUMP animation frames
       {0, 1, 2}                        // AI_SHOOT animation frames
    };  

    g_state.enemies = new Entity[ENEMY_COUNT];
    // walker (patrolling, no attack)
    g_state.enemies[2] = Entity(
        enemy_texture_id,
        0.0f,                   // speed
        0.8333f,                // width (15/18)
        1.0f,                   // height (18/18)
        ENEMY,                  // entity type
        JUMPER,                 // AI type
        JUMPING,                // AI State 
        AI_JUMP,                // AI Animation
        enemy_animations,
        0.7f,                   // jump power
        1,                      // animation frames
        6,                      // animation cols
        3,                      // animation rows
        acceleration
    );
    // jumper (stationary jump, no attack)
    g_state.enemies[1] = Entity(
        enemy_texture_id,
        1.0f,                   // speed
        0.8333f,                // width (15/18)
        1.0f,                   // height (18/18)
        ENEMY,                  // entity type
        WALKER,                 // AI type
        AI_IDLING,                // AI State 
        AI_IDLE,                // AI Animation
        enemy_animations,
        0.0f,                   // jump power
        4,                      // animation frames
        6,                      // animation cols
        3,                      // animation rows
        acceleration
    );
    // attacker (stationary, attack when player is nearby)
    g_state.enemies[0] = Entity(
        enemy_texture_id,
        1.25f,                   // speed
        0.8333f,                // width (15/18)
        1.0f,                   // height (18/18)
        ENEMY,                  // entity type
        ATTACKER,               // AI type
        AI_IDLING,              // AI State 
        AI_IDLE,               // AI Animation
        enemy_animations,
        0.0f,                   // jump power
        4,                      // animation frames
        6,                      // animation cols
        3,                      // animation rows
        acceleration
    );

    /* change individual positions i guess?*/
    g_state.enemies[0].set_position(glm::vec3(12.5f, 5.0f, 0.0f));
    g_state.enemies[1].set_position(glm::vec3(9.0f, 3.25f, 0.0f));
    g_state.enemies[2].set_position(glm::vec3(3.0f, 3.0f, 0.0f));

    
    // ––––– GENERAL ––––– //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{

    g_state.player->set_movement(glm::vec3(0.0f));
    
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_game_is_running = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        // Quit the game with a keystroke
                        g_game_is_running = false;
                        break;
                        
                    case SDLK_SPACE:
                        if (g_state.player->get_player_died()) break;

                        // Jump
                        if (g_state.player->get_collided_bottom())
                        {
                            g_state.player->jump();
                            g_state.player->set_animation_state(JUMP);                            
                            Mix_PlayChannel(-1, g_state.jump_sfx, 0);
                        }
                        break;
                        
                    case SDLK_h:
                        // Stop music
                        Mix_HaltMusic();
                        break;
                        
                    case SDLK_p:
                        Mix_PlayMusic(g_music, -1);
                        
                    default:
                        break;
                }
                
            default:
                break;
        }
    }
    if (g_state.player->get_player_died()) return;
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);
    g_state.player->set_animation_state(IDLE);
    if (key_state[SDL_SCANCODE_LEFT])
    {
        g_state.player->move_left();
    }
    else if (key_state[SDL_SCANCODE_RIGHT])
    {
        g_state.player->move_right();
    }
    
    if (glm::length(g_state.player->get_movement()) > 1.0f)
    {
        g_state.player->normalise_movement();
    }
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    delta_time += g_accumulator;
    
    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }

    if (g_state.player->get_player_died()) return;
    

    while (delta_time >= FIXED_TIMESTEP)
    {// float delta_time, Entity* player, Entity* collidable_entities, int collidable_entity_count, Map* map
        g_state.player->update(FIXED_TIMESTEP, NULL, g_state.enemies, ENEMY_COUNT, g_state.map);
        for (int i = 0; i < ENEMY_COUNT; i++) {
            g_state.enemies[i].update(FIXED_TIMESTEP, g_state.player, NULL, 0, g_state.map);
        }
        delta_time -= FIXED_TIMESTEP;
    }

    
    
    g_accumulator = delta_time;

    
    // Camera Follows the player
    g_view_matrix = glm::mat4(1.0f);
    g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-g_state.player->get_position().x, 2.75f, 0.0f));
}

void render()
{
    g_program.set_view_matrix(g_view_matrix);
    glClear(GL_COLOR_BUFFER_BIT);

    if (g_state.player->get_player_died()) {
        glm::vec3 text_pos = glm::vec3(g_state.player->get_position().x - 3.0f, 0.0f, 0.0f);
        draw_text(&g_program, g_font_id, "You Lose!", 0.4, 0.01f, text_pos);
    }

    if (g_state.player->get_enemies_killed() == ENEMY_COUNT) {
        glm::vec3 text_pos = glm::vec3(g_state.player->get_position().x - 3.0f, 0.0f, 0.0f);
        draw_text(&g_program, g_font_id, "You Win!", 0.4, 0.01f, text_pos);
    }

    g_state.player->render(&g_program);
    for (int i = 0; i < ENEMY_COUNT; i++) {
        g_state.enemies[i].render(&g_program);
    }
    g_state.map->render(&g_program);


    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();
    
    delete[]  g_state.enemies;
    delete    g_state.player;
    delete    g_state.map;
    Mix_FreeChunk(g_state.jump_sfx);
    Mix_FreeMusic(g_state.bgm);
}

// ––––– GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();
    
    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }
    
    shutdown();
    return 0;
}