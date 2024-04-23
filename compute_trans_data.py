from dataclasses import dataclass, field
from re import compile
from sys import argv, stderr
from typing import Any, Pattern, TextIO


@dataclass
class Class:
    name: str
    perms: set[str] = field(default_factory=set)

    def __hash__(self) -> int:
        return hash(self.name)

    def __eq__(self, other) -> bool:
        return isinstance(other, Class) and self.name == other.name

    def __str__(self) -> str:
        return self.name

    def __repr__(self) -> str:
        return self.name


@dataclass
class Type:
    name: str

    def __hash__(self) -> int:
        return hash(self.name)

    def __eq__(self, other) -> bool:
        return isinstance(other, Type) and self.name == other.name

    def __str__(self) -> str:
        return self.name

    def __repr__(self) -> str:
        return self.name


@dataclass
class Role:
    name: str
    types: set[Type] = field(default_factory=set)

    def __hash__(self) -> int:
        return hash(self.name)

    def __eq__(self, other) -> bool:
        return isinstance(other, Role) and self.name == other.name

    def __str__(self) -> str:
        return self.name

    def __repr__(self) -> str:
        return self.name


@dataclass
class User:
    name: str
    roles: set[Role] = field(default_factory=set)

    def __hash__(self) -> int:
        return hash(self.name)

    def __eq__(self, other) -> bool:
        return isinstance(other, User) and self.name == other.name

    def __str__(self) -> str:
        return self.name

    def __repr__(self) -> str:
        return self.name


@dataclass
class TypeTransition:
    src: Type
    tgt: Type
    cls: Class
    res: Type
    name: str | None = None


@dataclass
class Context:
    user: User
    role: Role
    type_: Type

    def __str__(self) -> str:
        return f"{self.user}:{self.role}:{self.type_}:s0"

    def __repr__(self) -> str:
        return str(self)


@dataclass
class Policy:
    commons: dict[str, set[str]] = field(default_factory=dict)
    classes: dict[str, Class] = field(default_factory=dict)
    types: dict[str, Type] = field(default_factory=dict)
    type_attributes: dict[str, set[Type]] = field(default_factory=dict)
    type_transitions: list[TypeTransition] = field(default_factory=list)
    roles: dict[str, Role] = field(
        default_factory=lambda: {"object_r": Role("object_r")}
    )
    users: dict[str, User] = field(default_factory=dict)


def class_def(policy: Policy, name: str) -> None:
    policy.classes[name] = Class(name)


def common_def(policy: Policy, name: str, perms: str) -> None:
    policy.commons[name] = set(perms.split())


def class_perm_def(
    policy: Policy, name: str, common: str | None = None, perms: str | None = None
) -> None:
    common_perms = policy.commons[common] if common in policy.commons else set()
    perms_list = set(perms.split()) if perms else set()
    policy.classes[name].perms.update(perms_list | common_perms)


def attribute_def(policy: Policy, name: str) -> None:
    policy.type_attributes[name] = set()


def type_def(policy: Policy, name: str) -> None:
    policy.types[name] = Type(name)


def typealias_def(policy: Policy, type_name: str, alias: str) -> None:
    policy.types[alias] = policy.types[type_name]


def typeattribute_def(policy: Policy, name: str, attrs: str) -> None:
    for attr in map(str.strip, attrs.split(",")):
        policy.type_attributes[attr].add(policy.types[name])


def role_def(policy: Policy, name: str) -> None:
    policy.roles[name] = Role(name)


def role_types_def(policy: Policy, name: str, types: str) -> None:
    policy.roles[name].types.update(
        (policy.types[type_name] for type_name in types.split())
    )


def type_transition_def(
    policy: Policy,
    src: str,
    tgt: str,
    cls: str,
    res: str,
    name: str | None = None,
    name_quoted: str | None = None,
) -> None:
    final_name = None
    if name_quoted:
        final_name = name_quoted
    if name:
        final_name = name
    policy.type_transitions.append(
        TypeTransition(
            policy.types[src],
            policy.types[tgt],
            policy.classes[cls],
            policy.types[res],
            final_name,
        )
    )


def user_def(
    policy: Policy, name: str, role: str | None = None, roles: str | None = None
) -> None:
    final_roles = []
    if role:
        final_roles.append(role)
    if roles:
        final_roles.extend(roles.split())
    policy.users[name] = User(name, set(policy.roles[role] for role in final_roles))


def parse(policy_file: TextIO) -> Policy:
    policy = Policy()
    defs: list[tuple[Pattern, Any]] = [
        (compile(r"^class (?P<name>\S+)$"), class_def),
        (compile(r"^common (?P<name>\S+) { (?P<perms>[\S ]+) }$"), common_def),
        (
            compile(
                r"^class (?P<name>\S+)(?: inherits (?P<common>\S+))?(?: { (?P<perms>[\S ]+) })?$"
            ),
            class_perm_def,
        ),
        (compile(r"^attribute (?P<name>\S+);$"), attribute_def),
        (compile(r"^type (?P<name>\S+);$"), type_def),
        (
            compile(r"^typealias (?P<type_name>\S+) alias (?P<alias>\S+);"),
            typealias_def,
        ),
        (
            compile(r"^typeattribute (?P<name>\S+) (?P<attrs>(?:\S+, )*\S+);$"),
            typeattribute_def,
        ),
        (
            compile(
                r"^type_transition (?P<src>\S+) (?P<tgt>\S+) ?: ?(?P<cls>\S+) (?P<res>\S+)"
                r'(?: \"(?P<name_quoted>[^"\r\n]+)\"| (?P<name>[^"]\S+[^"]))?;$'
            ),
            type_transition_def,
        ),
        (compile(r"^role (?P<name>\S+);"), role_def),
        (compile(r"^role (?P<name>\S+) types { (?P<types>[\S ]+) };"), role_types_def),
        (
            compile(
                r"^user (?P<name>\S+)"
                r"(?: roles (?P<role>[^{\s]+)| roles { (?P<roles>[^}]+) })?.*$"
            ),
            user_def,
        ),
    ]
    for line in policy_file:
        for pattern, def_fn in defs:
            if m := pattern.match(line):
                def_fn(policy, **m.groupdict())
                break
        else:
            assert not any(
                (
                    line.startswith("class "),
                    line.startswith("attribute "),
                    line.startswith("attribute "),
                    line.startswith("type "),
                    line.startswith("typealias "),
                    line.startswith("typeattribute "),
                    line.startswith("type_transition "),
                    line.startswith("role "),
                    line.startswith("user "),
                )
            ), f"Could not parse line {line}"
    return policy


def build_contexts(policy: Policy) -> dict[Type, Context]:
    role_user: dict[Role, User] = {
        policy.roles["object_r"]: policy.users["unconfined_u"]
    }
    contexts: dict[Type, Context] = {}
    for type_ in policy.types.values():
        roles = {key: role for key, role in policy.roles.items() if type_ in role.types}
        role = None
        if "unconfined_r" in roles:
            role = roles["unconfined_r"]
        elif "system_r" in roles:
            role = roles["system_r"]
        elif len(roles) > 0:
            role = list(roles.values())[0]
        else:
            role = policy.roles["object_r"]

        if role not in role_user:
            users = {
                key: user for key, user in policy.users.items() if role in user.roles
            }
            if "unconfined_u" in users:
                role_user[role] = users["unconfined_u"]
            elif "sysadm_u" in users:
                role_user[role] = users["sysadm_u"]
            elif len(users) > 0:
                role_user[role] = list(users.values())[0]
            else:
                print(f"No user for role {role} ({roles=}, {type_=})", file=stderr)
                continue
        user = role_user[role]
        contexts[type_] = Context(user, role, type_)

    return contexts


def print_bench_data(
    policy: Policy, contexts: dict[Type, Context], name_only: bool = False
) -> None:
    for trans in policy.type_transitions:
        if name_only and not trans.name:
            continue
        if trans.src not in contexts or trans.tgt not in contexts:
            continue
        print(contexts[trans.src], contexts[trans.tgt], trans.cls, end=" ")
        if trans.name:
            print(f'"{trans.name}"', end=" ")
        print(trans.res)


def main() -> None:
    name_only = False
    if len(argv) > 2 and argv[2] == "name-only":
        name_only = True
    with open(argv[1]) as policy_file:
        policy = parse(policy_file)
    print(f"commons: {len(policy.commons)}", file=stderr)
    print(f"classes: {len(policy.classes)}", file=stderr)
    print(f"types: {len(policy.types)}", file=stderr)
    print(f"type_attributes: {len(policy.type_attributes)}", file=stderr)
    print(f"type_transitions: {len(policy.type_transitions)}", file=stderr)
    print(f"roles: {len(policy.roles)}", file=stderr)
    print(f"users: {len(policy.users)}", file=stderr)
    contexts = build_contexts(policy)
    print_bench_data(policy, contexts, name_only)


if __name__ == "__main__":
    main()
