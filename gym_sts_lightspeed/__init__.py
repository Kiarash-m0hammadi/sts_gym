from gymnasium.envs.registration import register

register(
     id="STS-v0",
     entry_point="gym_sts_lightspeed.envs:StsEnv",
)
