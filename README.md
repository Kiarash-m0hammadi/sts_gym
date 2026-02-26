# sts_lightspeed

For tree search and simulation of the popular rogue-like deckbuilder game Slay The Spire.

**Features**
* C++ 17 compiled with GCC/Clang
* Standalone simulator
* Designed to be 100% RNG accurate
* Playable in console
* Speed: 1M random playouts in 5s with 16 threads
* Gymnasium environment for reinforcement learning

## Python API & Gymnasium Environment

`sts_lightspeed` provides a high-performance [Gymnasium](https://gymnasium.farama.org/) (formerly OpenAI Gym) environment for training AI agents.

### Installation

First, clone the repository and initialize submodules:

```bash
git clone https://github.com/keeganjebb/sts_lightspeed.git
cd sts_lightspeed
git submodule update --init --recursive
```

Then, install the Python package and build the C++ extension:

```bash
pip install .
```

Ensure you have a C++17 compatible compiler and CMake installed.

### Development

For detailed information on the environment structure, observation/action spaces, and how to contribute, please see [DEVELOPMENT.md](DEVELOPMENT.md).

### Usage

The environment is registered with the ID `STS-v0`.

```python
import gymnasium as gym
import gym_sts_lightspeed

env = gym.make("STS-v0", ascension=0)
obs, info = env.reset()

terminated = False
while not terminated:
    # Use action mask to sample valid actions
    action = env.action_space.sample(info['action_mask'])
    obs, reward, terminated, truncated, info = env.step(action)

    print(f"Floor: {info['floor']}, Reward: {reward}")
```

### Observation Space
The observation is a 1200-element integer vector representing the game state, including:
- Player HP, Gold, Floor, Act
- Deck composition
- Relics
- Hand, Draw pile, and Discard pile
- Combat state (Enemy HP, Intents, Blocks, etc.)

### Action Space
The action space is `Discrete(128)`. Since the number of valid actions varies by state, an `action_mask` is provided in the `info` dictionary to filter valid actions.

### Rewards
The default reward function includes:
- +10.0 for each floor progressed.
- ±1.0 for each point of HP change.
- +1000.0 for winning the game.
- -100.0 for losing the game.

## C++ Development

### Build tips
* If your build fails with an error about not-return-only `constexpr` methods, ensure your compiler supports C++17.
* CMake is used for building the project. For Python bindings, it is recommended to use the `setup.py` as described above.

## Implementation Progress
* All enemies
* All relics
* All Ironclad cards
* All colorless cards
* Most game events and non-combat rooms
