
#include <platform.h>
#include <timer.h>
#include <im.h>
#include <file.h>

#define SCREEN_LEFT -1280.0f/64.0f
#define SCREEN_RIGHT 1280.0f/64.0f
#define SCREEN_BOTTOM -720.0f/64.0f
#define SCREEN_TOP 720.0f/64.0f
#define SCREEN_WIDTH (SCREEN_RIGHT - SCREEN_LEFT)
#define SCREEN_HEIGHT (SCREEN_TOP - SCREEN_BOTTOM)

#define GRID_SIZE 64.0f

#define PLAYER_COLLIDER_SIZE 0.35f

#define BASE_ASTEROID_SCALE 2.5f

#define ASTEROID_DRIFT_SPEED 1.0f

#define ASTEROID_SHAPE_ENTROPY 2.0f

vec2_t line1 = {1.0f, 1.0f};
vec2_t line2 = {-1.0f, -1.0f};

vec4_t WHITE = {1, 1, 1, 1};
vec4_t RED = {1, 0, 0, 1};
vec4_t BLUE = {0, 0, 1, 1};
vec4_t CYAN = {0, 1, 1, 1};

typedef struct {
	vec2_t pos;
	vec2_t speed;
	float rotation;
	float animation;
} player_t;

typedef struct {
	vec2_t pos;
	float direction;
} bullet_t;

typedef struct {
	vec2_t pos;
	vec2_t p1;
	vec2_t p2;
	vec2_t speed;
	float rotation;
	float time;
} debris_t;

typedef struct {
	core_window_t window;
	core_timer_t timer;
	player_t player;
	dynarr_t bullets;
	dynarr_t debris;
	dynarr_t asteroids;
	int score;

	struct {
		m_arena memory;
		gfx_texture_t stars;
		gfx_texture_t stars2;
		gfx_texture_t font;
	} assets;
} game_t;

typedef struct {
	vec2_t pos;
	vec2_t startPt;
	dynarr_t pts;
	vec2_t speed;
	float scale;
	float collisionRadius;
} asteroid_t;


void* start() {
	game_t* game = malloc(sizeof(game_t));
	core_window(&game->window, "Asteroids", 1280, 720, 0);
	core_opengl(&game->window);
	core_timer(&game->timer);

	m_stack(&game->assets.memory, 0, 0);
	m_reserve(&game->assets.memory, GB(1), PAGE_SIZE);
	bitmap_t* stars = f_load_bitmap(&game->assets.memory, "stars.bmp");
	bitmap_t* stars2 = f_load_bitmap(&game->assets.memory, "stars2.bmp");
	game->assets.stars = gfx_create_texture(stars);
	game->assets.stars2 = gfx_create_texture(stars2);

	game->player.pos = vec2(5.0f, 0.0f);

	game->bullets = dynarr(sizeof(bullet_t));
	game->debris = dynarr(sizeof(debris_t));
	game->asteroids = dynarr(sizeof(asteroid_t));

	bitmap_t* font = f_load_font_file(&game->assets.memory, "font.bmp");
	game->assets.font = gfx_create_texture(font);
	game->score = 0;

	return game;
}

vec2_t pfront = {0.0f, 0.4f};
vec2_t pleft = {-0.3f, -0.4f};
vec2_t pright = {0.3f, -0.4f};
vec2_t pback1 = {-0.2f, -0.2f};
vec2_t pback2 = {0.2f, -0.2f};

vec2_t pengine1 = { 0.0f, -0.6f};
vec2_t pengine2 = {-0.1f, -0.4f};
vec2_t pengine3 = { 0.1f, -0.4f};

vec2_t rotate2(vec2_t v, float rads) {
	return vec2((-sinf(rads))*v.y + cosf(rads)*v.x, cosf(rads)*v.y + (sinf(rads))*v.x);
}

void add_debris(game_t* game, vec2_t pos, vec2_t speed, vec2_t l1, vec2_t l2) {
	debris_t d;
	d.p1 = l1;
	d.p2 = l2;
	d.pos = add2(pos, add2(l1, mul2f(sub2(l1, l2), 0.5f)));
	d.speed = add2(speed, vec2(r_float_range(-0.5f, 0.50f), r_float_range(-0.5f, 0.50f)));
	d.rotation = r_float(0.5f, 1.0f);
	d.time = r_float_range(5.0f, 20.0f);
	dynarr_push(&game->debris, &d);
}

void player_die(game_t* game) {
	vec2_t p = game->player.pos;
	add_debris(game, game->player.pos, game->player.speed, pfront, pleft);
	add_debris(game, game->player.pos, game->player.speed, pfront, pright);
	add_debris(game, game->player.pos, game->player.speed, pback1, pback2);

	game->player.pos = vec2(5.0f, 0.0f);
	game->player.speed = vec2(0.0f, 0.0f);
}

float er_float(float val, float entropy) {
	return r_float_range(val - entropy, val + entropy);
}

asteroid_t spawn_asteroid(float scale, vec2_t pos, float entropy) {
	asteroid_t asteroid;
	asteroid.pos = pos;
	asteroid.scale = scale;
	asteroid.pts = dynarr(sizeof(vec2_t));
	asteroid.speed = vec2(r_float_range(-ASTEROID_DRIFT_SPEED + 0.5f, ASTEROID_DRIFT_SPEED + 0.5f), r_float_range(-ASTEROID_DRIFT_SPEED + 0.5f, ASTEROID_DRIFT_SPEED + 0.5f));

	// for (int i = 0; i < 7; ++i) {
	// 	vec2_t pt = add2(asteroid.pos, vec2(0.0f/scale, 0.0f/scale));
	// 	dynarr_push(&asteroid.pts, &pt);
	// }
	// asteroid.pts = dynarr_push()
	
	vec2_t pt1 = vec2(er_float(-4.0f, entropy)/scale, er_float(3.0f, entropy)/scale);
	vec2_t pt2 = vec2(er_float(-1.0f, entropy)/scale, er_float(5.0f, entropy)/scale);
	vec2_t pt3 = vec2(er_float(3.0f, entropy)/scale, er_float(4.0f, entropy)/scale);
	vec2_t pt4 = vec2(er_float(4.0f, entropy)/scale, er_float(1.0f, entropy)/scale); 
	vec2_t pt5 = vec2(er_float(2.0f, entropy)/scale, er_float(-3.0f, entropy)/scale);
	vec2_t pt6 = vec2(er_float(-3.0f, entropy)/scale, er_float(-4.0f, entropy)/scale);
	vec2_t pt7 = vec2(er_float(-2.0f, entropy)/scale, er_float(0.0f, entropy)/scale);

	dynarr_push(&asteroid.pts, &pt1);
	dynarr_push(&asteroid.pts, &pt2);
	dynarr_push(&asteroid.pts, &pt3);
	dynarr_push(&asteroid.pts, &pt4);
	dynarr_push(&asteroid.pts, &pt5);
	dynarr_push(&asteroid.pts, &pt6);
	dynarr_push(&asteroid.pts, &pt7);

	return asteroid;
}

void destroy_asteroid(game_t* game, asteroid_t* asteroid) {
	if (asteroid->scale < BASE_ASTEROID_SCALE * 4)	{
		asteroid_t a = spawn_asteroid(asteroid->scale + BASE_ASTEROID_SCALE, add2f(asteroid->pos, 2.0f/asteroid->scale), ASTEROID_DRIFT_SPEED);
		asteroid_t b = spawn_asteroid(asteroid->scale + BASE_ASTEROID_SCALE, sub2f(asteroid->pos, 2.0f/asteroid->scale), ASTEROID_DRIFT_SPEED);
		dynarr_push(&game->asteroids, &a);
		dynarr_push(&game->asteroids, &b);
	}

	FORDYNARR (i, asteroid->pts) {
		vec2_t* pt = dynarr_get(&asteroid->pts, i);
		vec2_t* pt2 = dynarr_get(&asteroid->pts, (i+1) % asteroid->pts.count);
		add_debris(game, asteroid->pos, asteroid->speed, *pt, *pt2);
	}
}

b32 player_asteroid_collision(game_t* game, asteroid_t* asteroid) {
	player_t* player = &game->player;
	if (len2(sub2(player->pos, asteroid->pos)) < (asteroid->collisionRadius + PLAYER_COLLIDER_SIZE)) {
		player_die(game);
		// Split asteroid after successful asteroid/bullet collision
		destroy_asteroid(game, asteroid);
		return TRUE;
	}

	return FALSE;
}

b32 bullet_asteroid_collision(game_t* game, bullet_t* bullet, asteroid_t* asteroid) {
	if (len2(sub2(bullet->pos, asteroid->pos)) < (asteroid->collisionRadius + PLAYER_COLLIDER_SIZE)) {
		// Split asteroid after successful asteroid/bullet collision
		destroy_asteroid(game, asteroid);
		game->score += 1;
		return TRUE;
	}

	return FALSE;
}

void frame(game_t* game) {
	core_window_update(&game->window);
	core_timer_update(&game->timer);
	gfx_clear(vec4(0, 0, 0, 1));
	gfx_color(vec4(1, 1, 1, 1));

	float dt = game->timer.dt;

	// TODO: Move this to core_opengl: gfx_coord_system
	gfx_coord_system(1280.0f/64.0f, 720.0f/64.0f);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1280.0f/64.0f, 1280.0f/64.0f,
			-720.0f/64.0f, 720.0f/64.0f,
			-10.0f, 10.0f);
	glMatrixMode(GL_MODELVIEW);

	glPointSize(5.0f);

	// Background stars
	gfx_texture(&game->assets.stars2);
	gfx_color(vec4(0.5f, 0.5f, 0.5f, 1));
	gfx_sprite(&game->window, vec2(0, 0), 0, 0, 1280*2, 720*2, 0.5f);
	gfx_texture(NULL);
	gfx_color(vec4(1, 1, 1, 1));

	// glMatrixMode(GL_MODELVIEW);
	// glPushMatrix();
	// glTranslatef(-10.0f, 0.0f, 0.0f);
	// line1 = rotate2(line1, 1.0f * dt);
	// line2 = rotate2(line2, 1.0f * dt);
	// gfx_line(line1, line2);
	// glPopMatrix();

	// Update player
	player_t* player = &game->player;

	// Rotate ship with A and D keys
	if (game->window.keyboard[KEY_UP].down) {
		vec2_t dir = vec2(-sinf(player->rotation), cosf(player->rotation));
		player->speed = add2(player->speed, mul2f(dir, 10.0f * dt));
		player->animation += 8.0f * dt;
	} else {
		player->animation = 0.0f;
	}
	if (game->window.keyboard[KEY_LEFT].down) {
		player->rotation += 5.0f * dt;
	}
	if (game->window.keyboard[KEY_RIGHT].down) {
		player->rotation -= 5.0f * dt;
	}
	if (game->window.keyboard[KEY_SPACE].pressed) {
		bullet_t bullet;
		bullet.pos = add2(player->pos, rotate2(mul2f(pfront, 1.5f), player->rotation));
		bullet.direction = player->rotation;
		dynarr_push(&game->bullets, &bullet);
	}
	player->pos = add2(player->pos, mul2f(player->speed, dt));
	player->speed = mul2f(player->speed, 1.0f - (0.5f * dt));

	if (player->pos.x < SCREEN_LEFT) player->pos.x = SCREEN_RIGHT;
	if (player->pos.x > SCREEN_RIGHT) player->pos.x = SCREEN_LEFT;
	if (player->pos.y < SCREEN_BOTTOM) player->pos.y = SCREEN_TOP;
	if (player->pos.y > SCREEN_TOP) player->pos.y = SCREEN_BOTTOM;

	// Draw ship with thruster
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(player->pos.x, player->pos.y, 0.0f);
	glRotatef(player->rotation * (180/PI), 0.0f, 0.0f, 1.0f);
	gfx_line(pfront, pleft);
	gfx_line(pfront, pright);
	gfx_line(pback1, pback2);

	if (fmod(player->animation, 1.0f) > 0.3f) {
		gfx_line(pengine1, pengine2);
		gfx_line(pengine2, pengine3);
		gfx_line(pengine3, pengine1);
	}

	gfx_color(CYAN);
	gfx_line_circle(vec2(0, 0), PLAYER_COLLIDER_SIZE*2.0f, 10);
	gfx_color(WHITE);

	glPopMatrix();

	// Show thruster on w key press

	// Spawn bullet at ship point when space is pressed

	// Update bullets
	// gfx_line(vec2(0.0f, 0.0f), vec2(19.0f, 0.0f));
	FORDYNARR (i, game->bullets) {
		bullet_t* bullet = dynarr_get(&game->bullets, i);

		vec2_t dir = rotate2(vec2(0, 1), bullet->direction);//vec2(-sinf(bullet->direction), cosf(bullet->direction));
		vec2_t speed = mul2f(dir, 10.0f * dt);
		bullet->pos = add2(bullet->pos, speed);

		if (len2(sub2(player->pos, bullet->pos)) < PLAYER_COLLIDER_SIZE) {
			player_die(game);
		}

		// if (bullet->pos.x < -1280.0f/GRID_SIZE) bullet->pos.x = 1280.0f/GRID_SIZE;
		// if (bullet->pos.x > 1280.0f/GRID_SIZE) bullet->pos.x = -1280.0f/GRID_SIZE;
		// if (bullet->pos.y < -720.0f/GRID_SIZE) bullet->pos.y = 720.0f/GRID_SIZE;
		// if (bullet->pos.y > 720.0f/GRID_SIZE) bullet->pos.y = -720.0f/GRID_SIZE;

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glTranslatef(bullet->pos.x, bullet->pos.y, 0.0f);
		gfx_line(vec2(0, 0), vec2(0.0f, 0.07f));
		glPopMatrix();

		if (bullet->pos.x < -1280.0f/GRID_SIZE ||
			bullet->pos.x > 1280.0f/GRID_SIZE ||
			bullet->pos.y < -720.0f/GRID_SIZE ||
			bullet->pos.y > 720.0f/GRID_SIZE) {
			dynarr_pop(&game->bullets, i);
			--i;
		}
	}

	// Update debris
	FORDYNARR (i, game->debris) {
		debris_t* debris = dynarr_get(&game->debris, i);

		debris->time -= dt;

		if (debris->pos.x < -1280.0f/GRID_SIZE) debris->pos.x = 1280.0f/GRID_SIZE;
		if (debris->pos.x > 1280.0f/GRID_SIZE) debris->pos.x = -1280.0f/GRID_SIZE;
		if (debris->pos.y < -720.0f/GRID_SIZE) debris->pos.y = 720.0f/GRID_SIZE;
		if (debris->pos.y > 720.0f/GRID_SIZE) debris->pos.y = -720.0f/GRID_SIZE;

		debris->p1 = rotate2(debris->p1, debris->rotation * dt);
		debris->p2 = rotate2(debris->p2, debris->rotation * dt);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glTranslatef(debris->pos.x, debris->pos.y, 0.0f);
		debris->pos = add2(debris->pos, mul2f(debris->speed, dt));
		gfx_line(debris->p1, debris->p2);
		glPopMatrix();

		if (debris->time < 0.0f) {
			dynarr_pop(&game->debris, i);
			--i;
		}
	}

	// Draw asteroids
	if (game->asteroids.count == 0) {
		asteroid_t a = spawn_asteroid(2, vec2(r_float_range(SCREEN_LEFT, SCREEN_RIGHT), r_float_range(SCREEN_BOTTOM, SCREEN_TOP)), ASTEROID_DRIFT_SPEED);
		dynarr_push(&game->asteroids, &a);

		a = spawn_asteroid(2, vec2(r_float_range(SCREEN_LEFT, SCREEN_RIGHT), r_float_range(SCREEN_BOTTOM, SCREEN_TOP)), ASTEROID_DRIFT_SPEED);
		dynarr_push(&game->asteroids, &a);

		a = spawn_asteroid(2, vec2(r_float_range(SCREEN_LEFT, SCREEN_RIGHT), r_float_range(SCREEN_BOTTOM, SCREEN_TOP)), ASTEROID_DRIFT_SPEED);
		dynarr_push(&game->asteroids, &a);

		a = spawn_asteroid(2, vec2(r_float_range(SCREEN_LEFT, SCREEN_RIGHT), r_float_range(SCREEN_BOTTOM, SCREEN_TOP)), ASTEROID_DRIFT_SPEED);
		dynarr_push(&game->asteroids, &a);

		a = spawn_asteroid(2, vec2(r_float_range(SCREEN_LEFT, SCREEN_RIGHT), r_float_range(SCREEN_BOTTOM, SCREEN_TOP)), ASTEROID_DRIFT_SPEED);
		dynarr_push(&game->asteroids, &a);
	}

	FORDYNARR (i, game->asteroids) {
		asteroid_t* asteroid = dynarr_get(&game->asteroids, i);

		// gfx_color(CYAN);
		// gfx_point(asteroid->pos);
		// gfx_color(WHITE);

		vec2_t highest_point = vec2(0.0f, 0.0f);
		FORDYNARR (p, asteroid->pts) {
			vec2_t* pt = dynarr_get(&asteroid->pts, p);
			vec2_t* pt2 = dynarr_get(&asteroid->pts, (p+1) % asteroid->pts.count);

			if (len2(*pt) > len2(highest_point)) {
				highest_point = *pt;
			};

			asteroid->pos = add2(asteroid->pos, mul2f(asteroid->speed, dt/10));
			gfx_line(add2(*pt, asteroid->pos), add2(*pt2, asteroid->pos));
			
			if (asteroid->pos.x < -1280.0f/GRID_SIZE) asteroid->pos.x = 1280.0f/GRID_SIZE;
			if (asteroid->pos.x > 1280.0f/GRID_SIZE) asteroid->pos.x = -1280.0f/GRID_SIZE;
			if (asteroid->pos.y < -720.0f/GRID_SIZE) asteroid->pos.y = 720.0f/GRID_SIZE;
			if (asteroid->pos.y > 720.0f/GRID_SIZE) asteroid->pos.y = -720.0f/GRID_SIZE;
		}

		// gfx_color(CYAN);
		asteroid->collisionRadius = len2(highest_point);
		// gfx_line_circle(asteroid->pos, asteroid->collisionRadius * 2, 10);
		// gfx_color(WHITE); 
	}
		

	// generate new asteroid if bullet collides with asteroid

	FORDYNARR (j, game->asteroids) {
		asteroid_t* asteroid = dynarr_get(&game->asteroids, j);
	
		// Kill player after successful asteroid/player collision
		if (player_asteroid_collision(game, asteroid)) {
			
			dynarr_pop(&game->asteroids, j);
			--j; 
			break;
		}

		FORDYNARR (i, game->bullets) {
			bullet_t* bullet = dynarr_get(&game->bullets, i);
		
			// DETECT BULLET COLLISION
			if (bullet_asteroid_collision(game, bullet, asteroid)) {
				// Split asteroid
				dynarr_pop(&game->asteroids, j);
				--j;

				// Remove bullet
				dynarr_pop(&game->bullets, i);
				--i;
				break;
			}
		}
	}
	

	

	

	// Keep score top left of asteroids destroyed, 1 point per collision
	gfx_texture(&game->assets.font);
	gfx_color(RED);
	gfx_text(&game->window, vec2(SCREEN_LEFT + 1.0f, SCREEN_TOP - 1.5f), 5, "Asteroids: %i", game->score);

	
	// Audio

	core_opengl_swap(&game->window);
}

