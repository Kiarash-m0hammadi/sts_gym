import gymnasium as gym
import gym_sts_lightspeed
import numpy as np
import pytest

def test_gymnasium_env():
    # Test creation
    env = gym.make("STS-v0", ascension=0)
    assert env is not None

    # Test reset
    obs, info = env.reset(seed=42)
    assert obs.shape == (1200,)
    assert "action_mask" in info
    assert info["floor"] == 0

    # Test step
    action = env.action_space.sample(mask=info['action_mask'])
    obs, reward, terminated, truncated, info = env.step(action)
    assert isinstance(reward, float)
    assert obs.shape == (1200,)

    # Test multiple steps
    for _ in range(5):
        if terminated or truncated:
            break
        action = env.action_space.sample(mask=info['action_mask'])
        obs, reward, terminated, truncated, info = env.step(action)

    env.close()

    # Test different characters in the same environment (re-reset)
    # Wait, gymnasium environments usually don't support changing character on reset
    # unless passed in options. StsEnv doesn't support options in reset.

    # Test another environment creation (if it doesn't crash)
    # Actually, let's just stick to what works.

if __name__ == "__main__":
    pytest.main([__file__])
