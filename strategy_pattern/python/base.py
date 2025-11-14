from abc import ABC, ABCMeta, abstractmethod
from enum import Enum, auto
from dataclasses import dataclass, fields, field
from typing import Any, Dict, Optional, Type
import logging


class StrategyType(Enum):
    BASE = auto()
    CHAR = auto()


@dataclass(frozen=True)
class StrategyKey:
    type: StrategyType = field(init=False, default=StrategyType.BASE)

    def __bool__(self):
        """
        Returns False if all fields are None (wildcard), True otherwise
        """
        return any(getattr(self, _field.name) is not None for _field in fields(self))

    def match(self, other: object) -> bool:
        """
        Rule matching logic:
        A field set to None in **self** acts as a wildcard and matches any value in other.
        If other field is None, it does not match
        """
        if not isinstance(other, StrategyKey):
            return False

        for f in fields(self):
            self_val = getattr(self, f.name)
            other_val = getattr(other, f.name)
            # None -> wildcard *
            if self_val is not None and self_val != other_val:
                logging.debug(
                    f"f.name={f.name}, self_val={self_val}, other_val={other_val}"
                )
                return False
        return True


_STRATEGY_REGISTRY: Dict[StrategyKey, Type["BaseStrategy"]] = {}


class StrategyMeta(ABCMeta):
    def __new__(mcls, name: str, bases, namespace: Dict[str, Any]):
        cls = super().__new__(mcls, name, bases, namespace)
        key = namespace.get("key", None)
        # Skip BaseStrategy since it is also None
        if key is None:
            for base in bases:
                if isinstance(base, StrategyMeta):
                    key = getattr(base, "key", None)
                    # logging.debug(f"Base {base} has key {key}")
                    break
        if key is None:
            logging.error(f"Strategy {name} has no key")
            return cls

        # logging.debug(f"Registering strategy {name} with key {key}")
        if key in _STRATEGY_REGISTRY and bool(key):
            raise ValueError(
                f"Duplicate key={key} for ({name}) and existing registered strategy ({_STRATEGY_REGISTRY[key]})"
            )
        _STRATEGY_REGISTRY[key] = cls

        return cls


class BaseStrategy(ABC, metaclass=StrategyMeta):
    key: Optional[StrategyKey] = None

    @abstractmethod
    def before_attack(self) -> None: ...


class StrategyFactory:
    """
    Used to create strategy instances based on StrategyKey matching.
    """

    @staticmethod
    def get(input_key: StrategyKey) -> list[BaseStrategy]:
        matched = []

        for rule_key, cls in _STRATEGY_REGISTRY.items():
            if rule_key.match(input_key):
                matched.append(cls())

        return matched
