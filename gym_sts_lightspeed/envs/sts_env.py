import gymnasium as gym
from gymnasium import spaces
import numpy as np
import os
import sys

try:
    import slaythespire
except ImportError:
    # If not in path, try to suggest building it
    raise ImportError(
        "slaythespire module not found. "
        "Please ensure the C++ extension is built and added to your PYTHONPATH. "
        "You can usually do this by running 'make slaythespire' in the build directory "
        "and adding the build directory to PYTHONPATH."
    )

class StsEnv(gym.Env):
    MAX_ACTIONS = 128

    def __init__(self, ascension=0, character=None):
        super(StsEnv, self).__init__()

        self.ascension = ascension
        if character is None:
            self.character = slaythespire.CharacterClass.IRONCLAD
        else:
            self.character = character

        self.nn = slaythespire.getNNInterface()

        # Observation space: based on NNInterface (412 elements)
        self.observation_space = spaces.Box(
            low=0,
            high=np.array(self.nn.getObservationMaximums(), dtype=np.int32),
            dtype=np.int32
        )

        # Action space: discrete actions. We provide a mask in the info dict.
        self.action_space = spaces.Discrete(self.MAX_ACTIONS)

        self.gc = None
        self.bc = None
        self.in_combat = False
        self.last_hp = 0
        self.last_floor = 0
        self.available_actions = []

    def reset(self, seed=None, options=None):
        super().reset(seed=seed)

        if seed is None:
            # Slay the Spire uses uint64 seeds.
            # We use a 32-bit random number if no seed is provided to reset()
            seed = np.random.randint(0, 2**31)

        self.gc = slaythespire.GameContext(self.character, seed, self.ascension)
        self.bc = None
        self.in_combat = False

        self.last_hp = self.gc.cur_hp
        self.last_floor = self.gc.floor_num

        obs = self._get_observation()
        info = self._get_info()

        return obs, info

    def step(self, action_idx):
        if self.gc.outcome != slaythespire.GameOutcome.UNDECIDED:
             return self._get_observation(), 0.0, True, False, self._get_info()

        # Update info and available actions
        _ = self._get_info()

        if not self.available_actions:
             # If no actions available but game not over, something is wrong
             return self._get_observation(), 0.0, True, False, self._get_info()

        # Handle invalid action index
        if action_idx >= len(self.available_actions):
            # Penalty for invalid action
            reward = -1.0
            idx = 0 # Default to first action
        else:
            reward = 0
            idx = action_idx

        action = self.available_actions[idx]

        # Execute action
        if self.in_combat:
            action.execute(self.bc)
            # Check if battle is over
            if self.bc.outcome != slaythespire.BattleOutcome.UNDECIDED:
                self.bc.exit_battle(self.gc)
                self.in_combat = False
                self.bc = None
        else:
            action.execute(self.gc)
            # Check if we entered battle
            if self.gc.screen_state == slaythespire.ScreenState.BATTLE:
                self.in_combat = True
                self.bc = slaythespire.BattleContext()
                self.bc.init(self.gc)

        obs = self._get_observation()
        reward += self._get_reward()
        terminated = (self.gc.outcome != slaythespire.GameOutcome.UNDECIDED)
        truncated = False
        info = self._get_info()

        return obs, reward, terminated, truncated, info

    def _get_observation(self):
        # Pass both GameContext and BattleContext to NNInterface
        obs = np.array(self.nn.getObservation(self.gc, self.bc), dtype=np.int32)
        return obs

    def _get_info(self):
        if self.in_combat and self.bc:
            self.available_actions = self.bc.get_possible_actions()
            screen = "BATTLE"
            hp = self.bc.player.cur_hp # Use combat HP if available
        else:
            self.available_actions = self.gc.get_possible_actions()
            screen = str(self.gc.screen_state)
            hp = self.gc.cur_hp

        mask = np.zeros(self.MAX_ACTIONS, dtype=np.int8)
        mask[:min(len(self.available_actions), self.MAX_ACTIONS)] = 1

        return {
            "screen": screen,
            "action_mask": mask,
            "available_actions_count": len(self.available_actions),
            "floor": self.gc.floor_num,
            "hp": hp,
            "max_hp": self.gc.max_hp,
            "gold": self.gc.gold,
            "seed": self.gc.seed
        }

    def _get_reward(self):
        reward = 0

        # Reward for floor progress
        current_floor = self.gc.floor_num
        reward += (current_floor - self.last_floor) * 10.0
        self.last_floor = current_floor

        # Penalty for losing HP, reward for gaining HP
        # Use battle HP if in combat, else game HP
        if self.in_combat and self.bc:
            current_hp = self.bc.player.cur_hp
        else:
            current_hp = self.gc.cur_hp

        reward += (current_hp - self.last_hp) * 1.0
        self.last_hp = current_hp

        # Big reward/penalty for win/loss
        if self.gc.outcome == slaythespire.GameOutcome.PLAYER_VICTORY:
            reward += 1000.0
        elif self.gc.outcome == slaythespire.GameOutcome.PLAYER_LOSS:
            reward -= 100.0

        return reward
