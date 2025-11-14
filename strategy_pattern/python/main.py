import logging
import sys

logging.basicConfig(
    level=logging.DEBUG,
    format="%(asctime)s %(name)s %(levelname)s %(message)s",
    stream=sys.stdout,
)

from base import _STRATEGY_REGISTRY, StrategyFactory  # noqa: E402
from example_rules import CharacterStrategyKey  # noqa: E402

if __name__ == "__main__":
    logging.debug(f"STRATEGY_REGISTRY: {_STRATEGY_REGISTRY}")
    logging.info(
        f"CharacterStrategies: {StrategyFactory.get(CharacterStrategyKey(1, 2))}"
    )
