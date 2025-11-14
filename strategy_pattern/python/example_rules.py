from dataclasses import dataclass, field
import logging
from typing import Optional
from base import StrategyType, StrategyKey, BaseStrategy


@dataclass(frozen=True)
class CharacterStrategyKey(StrategyKey):
    type: StrategyType = field(init=False, default=StrategyType.CHAR)
    char_id: Optional[int]
    ai_level: Optional[int]


class CharacterStrategy(BaseStrategy):
    key = CharacterStrategyKey(None, None)  # wildcard for all

    def before_attack(self) -> None:
        logging.info(f"CharacterStrategy before_attack; key: {self.key}")


class ExampleStrategy(CharacterStrategy):
    key = CharacterStrategyKey(1, None)  # wildcard for all ai_levels


class ExampleStrategyEasy(CharacterStrategy):
    key = CharacterStrategyKey(1, 1)


class ExampleStrategyHard(CharacterStrategy):
    key = CharacterStrategyKey(1, 2)
