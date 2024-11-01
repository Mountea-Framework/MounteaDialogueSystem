from dataclasses import dataclass
from enum import Enum
from typing import Optional, List, Dict
import json
import unreal


class NodeType(Enum):
    FUNCTION_CALL = "function_call"
    CUSTOM_EVENT = "custom_event"


@dataclass
class NodeDefinition:
    type: NodeType
    class_name: str
    parent: str
    function: str
    is_interface_call: Optional[bool] = None

    def to_template(self) -> str:
        template = f'''Begin Object Class={self.class_name} Name="K2Node_MounteaDialogueCallFunction_1"'''
        if self.is_interface_call is not None:
            template += f'\n   bIsInterfaceCall={str(self.is_interface_call)}'
        template += f'\n   FunctionReference=(MemberParent=/Script/CoreUObject.Class'"{self.parent}"',MemberName="{self.function}")'
        return template

    @classmethod
    def from_dict(cls, data: Dict):
        return cls(
            type=NodeType(data['type']),
            class_name=data['class_name'],
            parent=data['parent'],
            function=data['function'],
            is_interface_call=data.get('is_interface_call')
        )


@dataclass
class NodeReplacement:
    old_node: NodeDefinition
    new_node: NodeDefinition

    def to_replacement_tuple(self) -> tuple[str, str]:
        return (self.old_node.to_template(), self.new_node.to_template())

    @classmethod
    def from_dict(cls, data: Dict):
        return cls(
            old_node=NodeDefinition.from_dict(data['old_node']),
            new_node=NodeDefinition.from_dict(data['new_node'])
        )


class NodeReplacementManager:
    def __init__(self):
        self.replacements: List[NodeReplacement] = []

    def load_from_json(self, file_path: str):
        with open(file_path, 'r') as f:
            data = json.load(f)
            self.replacements = [NodeReplacement.from_dict(item) for item in data['replacements']]

    def get_replacements(self) -> List[tuple[str, str]]:
        return [replacement.to_replacement_tuple() for replacement in self.replacements]