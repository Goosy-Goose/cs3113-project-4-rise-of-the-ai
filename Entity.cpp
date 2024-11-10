#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "Entity.h"

void Entity::ai_activate(Entity* player, Map* map)
{
    switch (m_ai_type)
    {
    case WALKER:
        ai_walk(map);
        break;

    case ATTACKER:
        ai_guard(player);
        break;
    case JUMPER:
        ai_jump(map);
        break;
    default:
        break;
    }
}

void Entity::ai_walk(Map* map)
{
    switch (m_ai_state) {
    case AI_IDLING:
        break;
    case AI_WALKING_R:
        set_animation_state(AI_RIGHT);
        m_movement = glm::vec3(1.0f, 0.0f, 0.0f);
        break;
    case AI_WALKING_L:
        set_animation_state(AI_LEFT);
        m_movement = glm::vec3(-1.0f, 0.0f, 0.0f);
    default:
        break;
    }
}

void Entity::ai_guard(Entity* player)
{
    switch (m_ai_state) {
    case AI_IDLING:
        if (glm::distance(m_position, player->get_position()) < 4.0f) m_ai_state = ATTACKING;
        break;

    
    case ATTACKING:
        if (m_position.x > player->get_position().x) {
            m_movement = glm::vec3(-1.0f, 0.0f, 0.0f);
            set_animation_state(AI_LEFT);
        }
        else {
            m_movement = glm::vec3(1.0f, 0.0f, 0.0f);
            set_animation_state(AI_RIGHT);
        }
        break;

    default:
        break;
    }
}

void Entity::ai_jump(Map* map) 
{

    if (!m_is_jumping) {
        m_is_jumping = true;
        m_velocity.y += m_jumping_power;
    }
}

// Default constructor
Entity::Entity()
    : m_position(0.0f), m_movement(0.0f), m_scale(1.0f, 1.0f, 0.0f), m_model_matrix(1.0f),
    m_speed(0.0f), m_animation_cols(0), m_animation_frames(0), m_animation_index(0), 
    m_animation_rows(0), m_animation_indices(nullptr), m_animation_time(0.0f), m_current_animation(IDLE),
    m_texture_id(0), m_velocity(0.0f), m_acceleration(0.0f), m_width(0.0f), m_height(0.0f)
{
}

// Parameterized constructor
Entity::Entity(GLuint texture_id, float speed, glm::vec3 acceleration, float jump_power, std::vector<std::vector<int>> animations, PlayerAnimation animation, float animation_time, 
               int animation_frames, int animation_index, int animation_cols, int animation_rows, float width, float height, EntityType EntityType)
  : m_position(0.0f), m_movement(0.0f), m_scale(1.0f, 1.0f, 0.0f), m_model_matrix(1.0f),
    m_speed(speed), m_acceleration(acceleration), m_jumping_power(jump_power), 
    m_animation_cols(animation_cols), m_animation_frames(animation_frames), m_animation_index(animation_index),
    m_animation_rows(animation_rows), m_animation_indices(nullptr), m_current_animation(animation), m_animations(animations),
    m_animation_time(animation_time), m_texture_id(texture_id), m_velocity(0.0f),
    m_width(width), m_height(height), m_entity_type(EntityType)
{
    set_animation_state(m_current_animation);
    //set_walking(animations);
}

// Simpler constructor for partial initialization
Entity::Entity(GLuint texture_id, float speed, float width, float height, EntityType EntityType)
    : m_position(0.0f), m_movement(0.0f), m_scale(1.0f, 1.0f, 0.0f), m_model_matrix(1.0f),
    m_speed(speed), m_animation_cols(0), m_animation_frames(0), m_animation_index(0),
    m_animation_rows(0), m_animation_indices(nullptr), m_animation_time(0.0f), m_current_animation(IDLE),
    m_texture_id(texture_id), m_velocity(0.0f), m_acceleration(0.0f), m_width(width), m_height(height), m_entity_type(EntityType)
{
    set_animation_state(m_current_animation);
}

// ai constructor
    // ai needs jump power, animation time, frame amount,animation col amt, animation row amt
Entity::Entity(GLuint texture_id, float speed, float width, float height, EntityType EntityType, AIType AIType, AIState AIState, AIAnimation AIAnim, std::vector<std::vector<int>> animations,
    float jump_power, int animation_frames, int animation_cols, int animation_rows, glm::vec3 acceleration)
    : m_position(0.0f), m_movement(0.0f), m_scale(1.0f, 1.0f, 0.0f), m_model_matrix(1.0f), m_jumping_power(jump_power), m_is_jumping(false),
      m_speed(speed), m_animation_cols(animation_cols), m_animation_frames(animation_frames), m_animation_index(0),
      m_animation_rows(animation_rows), m_animation_indices(nullptr), m_animation_time(0.0f),
      m_texture_id(texture_id), m_velocity(0.0f), m_acceleration(acceleration), m_width(width), 
      m_height(height), m_entity_type(EntityType), m_ai_type(AIType), m_ai_state(AIState), m_ai_current_animation(AIAnim), m_animations(animations)
{
    set_animation_state(m_ai_current_animation);
}

Entity::~Entity() { }


void Entity::set_animation_state(PlayerAnimation new_animation)
{
    m_current_animation = new_animation;
    // Update the texture and animation indices based on the current animation
    m_animation_indices = m_animations[m_current_animation].data();
    // Update the number of rows to match the new texture spritesheet
    m_animation_frames = (int)m_animations[m_current_animation].size();
}

void Entity::set_animation_state(AIAnimation new_animation)
{
    m_ai_current_animation = new_animation;
    m_animation_indices = m_animations[m_ai_current_animation].data();
    m_animation_frames = (int)m_animations[m_ai_current_animation].size();

}


void Entity::draw_sprite_from_texture_atlas(ShaderProgram* program, GLuint texture_id, int index)
{
    // Step 1: Calculate the UV location of the indexed frame
    float u_coord = (float)(index % m_animation_cols) / (float)m_animation_cols;
    float v_coord = (float)(index / m_animation_cols) / (float)m_animation_rows;

    // Step 2: Calculate its UV size
    float width = 1.0f / (float)m_animation_cols;
    float height = 1.0f / (float)m_animation_rows;

    // Step 3: Just as we have done before, match the texture coordinates to the vertices
    float tex_coords[] =
    {
        u_coord, v_coord + height, u_coord + width, v_coord + height, u_coord + width, v_coord,
        u_coord, v_coord + height, u_coord + width, v_coord, u_coord, v_coord
    };

    float vertices[] =
    {
        -0.5, -0.5, 0.5, -0.5,  0.5, 0.5,
        -0.5, -0.5, 0.5,  0.5, -0.5, 0.5
    };

    // Step 4: And render
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->get_position_attribute());

    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

bool const Entity::check_collision(Entity* other) const
{
    if (!other->m_is_active) return false;
    float x_distance = fabs(m_position.x - other->m_position.x) - ((m_width + other->m_width) / 2.0f);
    float y_distance = fabs(m_position.y - other->m_position.y) - ((m_height + other->m_height) / 2.0f);

    return x_distance < 0.0f && y_distance < 0.0f;
}

void const Entity::check_collision_y(Entity* collidable_entities, int collidable_entity_count)
{
    for (int i = 0; i < collidable_entity_count; i++)
    {
        Entity* collidable_entity = &collidable_entities[i];
        if (!collidable_entity->m_is_active) break;

        if (check_collision(collidable_entity))
        {
            float y_distance = fabs(m_position.y - collidable_entity->m_position.y);
            float y_overlap = fabs(y_distance - (m_height / 2.0f) - (collidable_entity->m_height / 2.0f));
            if (m_velocity.y > 0)
            {
                m_position.y -= y_overlap;
                m_velocity.y = 0;

                // Collision!
                m_collided_top = true;
            }
            else if (m_velocity.y < 0)
            {
                m_position.y += y_overlap;
                float old_yvel = m_velocity.y;
                m_velocity.y = 0;

                // Collision!
                m_collided_bottom = true;
                if (m_entity_type == PLAYER && collidable_entity->m_entity_type == ENEMY) {
                    collidable_entity->deactivate();
                    m_num_enemies_killed++;
                }
            }
        }
    }
}

void const Entity::check_collision_x(Entity* collidable_entities, int collidable_entity_count)
{
    for (int i = 0; i < collidable_entity_count; i++)
    {
        Entity* collidable_entity = &collidable_entities[i];
        if (!collidable_entity->m_is_active) break;


        if (check_collision(collidable_entity))
        {
            float x_distance = fabs(m_position.x - collidable_entity->m_position.x);
            float x_overlap = fabs(x_distance - (m_width / 2.0f) - (collidable_entity->m_width / 2.0f));
            if (m_velocity.x > 0)
            {
                m_position.x -= x_overlap;
                m_velocity.x = 0;

                // Collision!
                m_collided_right = true;

            }
            else if (m_velocity.x < 0)
            {
                m_position.x += x_overlap;
                m_velocity.x = 0;

                // Collision!
                m_collided_left = true;
            }
        }
    }
}


void const Entity::check_collision_y(Map* map)
{
    if (!m_is_active) return;
    // Probes for tiles above
    glm::vec3 top = glm::vec3(m_position.x, m_position.y + (m_height / 2), m_position.z);
    glm::vec3 top_left = glm::vec3(m_position.x - (m_width / 2), m_position.y + (m_height / 2), m_position.z);
    glm::vec3 top_right = glm::vec3(m_position.x + (m_width / 2), m_position.y + (m_height / 2), m_position.z);

    // Probes for tiles below
    glm::vec3 bottom = glm::vec3(m_position.x, m_position.y - (m_height / 2), m_position.z);
    glm::vec3 bottom_left = glm::vec3(m_position.x - (m_width / 2) - 0.1, m_position.y - (m_height / 2), m_position.z);
    glm::vec3 bottom_right = glm::vec3(m_position.x + (m_width / 2) + 0.1, m_position.y - (m_height / 2), m_position.z);

    float penetration_x = 0;
    float penetration_y = 0;

    // If the map is solid, check the top three points
    if (map->is_solid(top, &penetration_x, &penetration_y) && m_velocity.y > 0)
    {
        m_position.y -= penetration_y;
        m_velocity.y = 0;
        m_collided_top = true;
    }
    else if (map->is_solid(top_left, &penetration_x, &penetration_y) && m_velocity.y > 0)
    {
        m_position.y -= penetration_y;
        m_velocity.y = 0;
        m_collided_top = true;
    }
    else if (map->is_solid(top_right, &penetration_x, &penetration_y) && m_velocity.y > 0)
    {
        m_position.y -= penetration_y;
        m_velocity.y = 0;
        m_collided_top = true;
    }

    // And the bottom three points
    if (map->is_solid(bottom, &penetration_x, &penetration_y) && m_velocity.y < 0)
    {
        m_position.y += penetration_y;
        m_velocity.y = 0;
        m_collided_bottom = true;
        if (!map->is_solid(bottom_left, &penetration_x, &penetration_y) && m_entity_type == ENEMY) {
            m_enemy_left_pit_detected = true;
        }
        if (!map->is_solid(bottom_right, &penetration_x, &penetration_y) && m_entity_type == ENEMY) {
            m_enemy_right_pit_detected = true;
        }
    }
    else if (map->is_solid(bottom_left, &penetration_x, &penetration_y) && m_velocity.y < 0)
    {
        m_position.y += penetration_y;
        m_velocity.y = 0;
        m_collided_bottom = true;
        
    }
    else if (map->is_solid(bottom_right, &penetration_x, &penetration_y) && m_velocity.y < 0)
    {
        m_position.y += penetration_y;
        m_velocity.y = 0;
        m_collided_bottom = true;
        if (m_entity_type == ENEMY) {
            if (!map->is_solid(bottom_right, &penetration_x, &penetration_y))
                m_enemy_right_pit_detected = true;
        }
        
    }
    
    //if (m_collided_bottom){ m_is_jumping = false; }
}

void const Entity::check_collision_x(Map* map)
{
    if (!m_is_active) return;
    // Probes for tiles; the x-checking is much simpler
    glm::vec3 left = glm::vec3(m_position.x - (m_width / 2), m_position.y, m_position.z);
    glm::vec3 right = glm::vec3(m_position.x + (m_width / 2), m_position.y, m_position.z);

    float penetration_x = 0;
    float penetration_y = 0;

    if (map->is_solid(left, &penetration_x, &penetration_y) && m_velocity.x < 0)
    {
        m_position.x += penetration_x;
        m_velocity.x = 0;
        m_collided_left = true;
    }
    if (map->is_solid(right, &penetration_x, &penetration_y) && m_velocity.x > 0)
    {
        m_position.x -= penetration_x;
        m_velocity.x = 0;
        m_collided_right = true;
    }
}



void Entity::update(float delta_time, Entity* player, Entity* collidable_entities, int collidable_entity_count, Map* map)
{
    if (!m_is_active) return;

    m_collided_top = false;
    m_collided_bottom = false;
    m_collided_left = false;
    m_collided_right = false;

    m_enemy_left_pit_detected = false;
    m_enemy_right_pit_detected = false;


    if (m_animation_indices != NULL)
    {
        m_animation_time += delta_time;
        float frames_per_second = (float)1 / SECONDS_PER_FRAME;

        if (m_animation_time >= frames_per_second)
        {
            m_animation_time = 0.0f;
            m_animation_index++;

            if (m_animation_index >= m_animation_frames)
            {
                m_animation_index = 0;
            }
        }
    }

    m_velocity.x = m_movement.x * m_speed;
    m_velocity += m_acceleration * delta_time;

    m_position.x += m_velocity.x * delta_time;
    check_collision_x(collidable_entities, collidable_entity_count);
    /* if player hits enemy from side */ 
    if (m_entity_type == PLAYER && (m_collided_left || m_collided_right)) {
        m_player_died = true;
    }
    check_collision_x(map);

    m_position.y += m_velocity.y * delta_time;
    check_collision_y(collidable_entities, collidable_entity_count);
    check_collision_y(map);   
    
    if (m_entity_type == ENEMY) ai_activate(player, map);
    if (m_ai_type == WALKER && m_ai_state == AI_IDLING) {
        if (m_collided_bottom) m_ai_state == AI_WALKING_L;
    }
    
    // ------ JUMP ----- //
    /* if the enemy touches the floor, jump */
    if (m_entity_type == ENEMY && m_ai_type == JUMPER) {
        if (m_collided_bottom) 
        {
            m_is_jumping = false;
        }
    }
    /* if the player (in main) jumps */
    else if (m_entity_type == PLAYER) {
        if (m_is_jumping)
        {
            m_is_jumping = false;
            m_velocity.y += m_jumping_power;
        }
    }

    if (m_entity_type == ENEMY && m_ai_type == WALKER) {
        if (m_ai_state == AI_IDLING) m_ai_state = AI_WALKING_L;
        if (!m_collided_bottom) { m_movement = glm::vec3(0.0f, 0.0f, 0.0f); }
        
        if (m_enemy_left_pit_detected || m_collided_left) {
            m_ai_state = AI_WALKING_R;
        }
        else if (m_enemy_right_pit_detected || m_collided_right) {
            m_ai_state = AI_WALKING_L;
        }
    }

    

    m_model_matrix = glm::mat4(1.0f);
    m_model_matrix = glm::translate(m_model_matrix, m_position);
    m_model_matrix = glm::scale(m_model_matrix, m_scale);
}


void Entity::render(ShaderProgram* program)
{
    if (!m_is_active) return;
    program->set_model_matrix(m_model_matrix);

    if (m_animation_indices != NULL)
    {
        draw_sprite_from_texture_atlas(program, m_texture_id, m_animation_indices[m_animation_index]);
        return;
    }

    float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
    float tex_coords[] = { 0.0,  1.0, 1.0,  1.0, 1.0, 0.0,  0.0,  1.0, 1.0, 0.0,  0.0, 0.0 };

    glBindTexture(GL_TEXTURE_2D, m_texture_id);

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}