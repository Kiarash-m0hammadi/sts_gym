import gymnasium as gym
import gym_sts_lightspeed
import random

def main():
    # Initialize the environment
    # Note: ensure that 'build' is in PYTHONPATH or sys.path
    env = gym.make("STS-v0", ascension=0)

    for episode in range(5):
        obs, info = env.reset(seed=episode + 100)
        terminated = False
        truncated = False
        total_reward = 0
        steps = 0

        print(f"--- Episode {episode + 1} ---")

        while not (terminated or truncated):
            # Choose a random action from the available ones
            # In our environment, any action index is mapped to a valid action via wrap-around,
            # but we can also use info['available_actions_count']
            num_actions = info["available_actions_count"]
            if num_actions == 0:
                print("No actions available. Ending episode.")
                break

            action = random.randint(0, num_actions - 1)

            obs, reward, terminated, truncated, info = env.step(action)
            total_reward += reward
            steps += 1

            if steps % 100 == 0:
                print(f"Step {steps}: Reward={total_reward:.1f}, HP={info['hp']}, Floor={info['floor']}, Screen={info['screen']}")

        print(f"Episode {episode + 1} finished in {steps} steps. Total Reward: {total_reward:.1f}, Final Floor: {info['floor']}, Outcome: {'Win' if total_reward > 500 else 'Loss'}")
        print()

if __name__ == "__main__":
    main()
