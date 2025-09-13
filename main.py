import pygame
import sys
import math

WIDTH, HEIGHT = 960, 720
BG_COLOR = (30, 30, 30)
BOX1_COLOR = (220, 50, 50)
BOX2_COLOR = (50, 50, 220)
BALL_COLOR = (230, 230, 80)

BOX_SIZE = 50
SPEED = 500

BALL_RADIUS = 22
BALL_START_VEL = (250, -180)
BALL_BOUNCE_DAMP = 0.95  # energy loss on bounce
BALL_FRICTION = 0.995  # slow down over time


def resolve_aabb_overlap(mover: pygame.Rect, static: pygame.Rect, vx: float, vy: float):
    """
    Push 'mover' rect out of 'static' along the smallest overlap axis.
    vx, vy are the mover's intended movement this frame; used to bias resolution
    so we push against the movement direction when overlaps are equal.
    Returns dx, dy correction that was applied to mover.
    """
    if not mover.colliderect(static):
        return 0, 0

    # Compute overlaps
    overlap_left = mover.right - static.left
    overlap_right = static.right - mover.left
    overlap_top = mover.bottom - static.top
    overlap_bottom = static.bottom - mover.top

    # Choose the smallest axis to resolve
    min_x_overlap = overlap_left if overlap_left < overlap_right else -overlap_right
    min_y_overlap = overlap_top if overlap_top < overlap_bottom else -overlap_bottom

    # Bias by movement direction to avoid corner sticking
    if abs(min_x_overlap) < abs(min_y_overlap):
        dx = -min_x_overlap
        dy = 0
    elif abs(min_y_overlap) < abs(min_x_overlap):
        dx = 0
        dy = -min_y_overlap
    else:
        # equal overlap: use velocity bias
        if abs(vx) >= abs(vy):
            dx = -min_x_overlap
            dy = 0
        else:
            dx = 0
            dy = -min_y_overlap

    mover.x += int(dx)
    mover.y += int(dy)
    return int(dx), int(dy)


def circle_rect_overlap(cx, cy, r, rect: pygame.Rect):
    """
    Check circle-rect overlap and return the minimum push-out vector (px, py)
    to separate the circle from the rect. If no overlap, returns (0, 0).
    """
    # Closest point on rect to circle center
    closest_x = max(rect.left, min(cx, rect.right))
    closest_y = max(rect.top, min(cy, rect.bottom))

    dx = cx - closest_x
    dy = cy - closest_y
    dist_sq = dx * dx + dy * dy
    if dist_sq > r * r:
        return 0.0, 0.0

    dist = math.sqrt(dist_sq) if dist_sq != 0 else 0.0

    # If center is exactly at corner, push out along the smallest axis
    if dist == 0.0:
        # Choose shortest push to get out: toward nearest edge
        left_dist = abs(cx - rect.left)
        right_dist = abs(rect.right - cx)
        top_dist = abs(cy - rect.top)
        bottom_dist = abs(rect.bottom - cy)
        # Push toward smallest distance
        options = [
            (-(r - left_dist), 0.0),  # push left
            ((r - right_dist), 0.0),  # push right
            (0.0, -(r - top_dist)),  # push up
            (0.0, (r - bottom_dist)),  # push down
        ]
        px, py = min(options, key=lambda v: abs(v[0]) + abs(v[1]))
        return px, py

    nx = dx / dist
    ny = dy / dist
    # Penetration depth
    pen = r - dist
    px = nx * pen
    py = ny * pen
    return px, py


def reflect_velocity_over_normal(vx, vy, nx, ny, damp=1.0):
    # v' = v - 2*(v·n)n
    dot = vx * nx + vy * ny
    rx = vx - 2 * dot * nx
    ry = vy - 2 * dot * ny
    return rx * damp, ry * damp


def main():
    pygame.init()
    screen = pygame.display.set_mode((WIDTH, HEIGHT))
    pygame.display.set_caption("Boxes + Ball")
    clock = pygame.time.Clock()

    # Float positions for precise movement
    x1 = (WIDTH - BOX_SIZE) / 1.2
    y1 = (HEIGHT - BOX_SIZE) / 2
    x2 = (WIDTH - BOX_SIZE) / 4
    y2 = (HEIGHT - BOX_SIZE) / 2

    box1 = pygame.Rect(int(x1), int(y1), BOX_SIZE, BOX_SIZE)
    box2 = pygame.Rect(int(x2), int(y2), BOX_SIZE, BOX_SIZE)

    # Ball state
    ball_pos = pygame.Vector2(WIDTH * 0.5, HEIGHT * 0.5)
    ball_vel = pygame.Vector2(BALL_START_VEL)
    ball_r = BALL_RADIUS

    running = True
    while running:
        dt = clock.tick(60) / 1000.0

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

        keys = pygame.key.get_pressed()

        # Box 1 input
        dx1 = keys[pygame.K_RIGHT] - keys[pygame.K_LEFT]
        dy1 = keys[pygame.K_DOWN] - keys[pygame.K_UP]
        if dx1 and dy1:
            inv = 1 / 1.41421356237
            dx1 *= inv
            dy1 *= inv
        x1 += dx1 * SPEED * dt
        y1 += dy1 * SPEED * dt

        # Clamp and sync
        x1 = max(0, min(x1, WIDTH - BOX_SIZE))
        y1 = max(0, min(y1, HEIGHT - BOX_SIZE))
        box1.topleft = (int(x1), int(y1))

        # Box 2 input
        dx2 = keys[pygame.K_d] - keys[pygame.K_a]
        dy2 = keys[pygame.K_s] - keys[pygame.K_w]
        if dx2 and dy2:
            inv = 1 / 1.41421356237
            dx2 *= inv
            dy2 *= inv
        x2 += dx2 * SPEED * dt
        y2 += dy2 * SPEED * dt

        # Clamp and sync
        x2 = max(0, min(x2, WIDTH - BOX_SIZE))
        y2 = max(0, min(y2, HEIGHT - BOX_SIZE))
        box2.topleft = (int(x2), int(y2))

        # Resolve box-box overlap (both are "movers", do two-way)
        if box1.colliderect(box2):
            # Resolve from perspective of box1 vs box2, bias by box1 movement
            r1 = box1.copy()
            resolve_aabb_overlap(r1, box2, dx1, dy1)
            # Resolve from perspective of box2 vs box1, bias by box2 movement
            r2 = box2.copy()
            resolve_aabb_overlap(r2, box1, dx2, dy2)
            # Choose the one with smaller total correction to avoid oscillation
            corr1 = abs(r1.x - box1.x) + abs(r1.y - box1.y)
            corr2 = abs(r2.x - box2.x) + abs(r2.y - box2.y)
            if corr1 + corr2 == 0:
                # Fallback: separate along x based on movement
                if abs(dx1) + abs(dx2) >= abs(dy1) + abs(dy2):
                    if dx1 > dx2:
                        box1.right = box2.left
                    else:
                        box2.right = box1.left
                else:
                    if dy1 > dy2:
                        box1.bottom = box2.top
                    else:
                        box2.bottom = box1.top
            else:
                box1.topleft = r1.topleft
                box2.topleft = r2.topleft
            # Sync floats
            x1, y1 = float(box1.x), float(box1.y)
            x2, y2 = float(box2.x), float(box2.y)

        # Ball physics
        ball_pos += ball_vel * dt
        ball_vel *= BALL_FRICTION

        # Ball vs window bounds
        if ball_pos.x - ball_r < 0:
            ball_pos.x = ball_r
            ball_vel.x = -ball_vel.x * BALL_BOUNCE_DAMP
        if ball_pos.x + ball_r > WIDTH:
            ball_pos.x = WIDTH - ball_r
            ball_vel.x = -ball_vel.x * BALL_BOUNCE_DAMP
        if ball_pos.y - ball_r < 0:
            ball_pos.y = ball_r
            ball_vel.y = -ball_vel.y * BALL_BOUNCE_DAMP
        if ball_pos.y + ball_r > HEIGHT:
            ball_pos.y = HEIGHT - ball_r
            ball_vel.y = -ball_vel.y * BALL_BOUNCE_DAMP

        # Ball vs boxes collision (push out and reflect velocity along normal)
        for rect, mvx, mvy in [
            (box1, dx1 * SPEED, dy1 * SPEED),
            (box2, dx2 * SPEED, dy2 * SPEED),
        ]:
            px, py = circle_rect_overlap(ball_pos.x, ball_pos.y, ball_r, rect)
            if px or py:
                # Separate
                ball_pos.x += px
                ball_pos.y += py

                # Compute normal
                nlen = math.hypot(px, py)
                if nlen == 0:
                    nx, ny = 0.0, -1.0
                else:
                    nx, ny = px / nlen, py / nlen

                # Relative velocity of ball against moving box (boxes are kinematic)
                rel_vx = ball_vel.x - mvx
                rel_vy = ball_vel.y - mvy
                rvx, rvy = reflect_velocity_over_normal(
                    rel_vx, rel_vy, nx, ny, BALL_BOUNCE_DAMP
                )
                # Convert back to world velocity
                ball_vel.x = rvx + mvx
                ball_vel.y = rvy + mvy

        # Draw
        screen.fill(BG_COLOR)
        pygame.draw.rect(screen, BOX1_COLOR, box1)
        pygame.draw.rect(screen, BOX2_COLOR, box2)
        pygame.draw.circle(
            screen, BALL_COLOR, (int(ball_pos.x), int(ball_pos.y)), ball_r
        )
        pygame.display.flip()

    pygame.quit()
    sys.exit()


if __name__ == "__main__":
    main()
