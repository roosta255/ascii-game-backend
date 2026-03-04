import re
import os
from collections import defaultdict

# Configuration: Hardcoded paths
DEFINITIONS_FILE = "./src/traits/Trait.enum"
USAGE_FILES = [
    "./src/role/Role.enum",
    "./src/traits/Trait.enum", 
    "./src/inventory/Item.enum",
    "./src/chest/Critter.enum"
]

def audit():
    # Regex to find any word starting with TRAIT_
    trait_usage_regex = re.compile(r'TRAIT_[A-Z0-9_]+')
    
    # Regex to find declarations: TRAIT_DECL(NAME, ...)
    trait_decl_regex = re.compile(r'TRAIT_DECL\s*\(\s*([A-Z0-9_]+)')

    declared_traits = set()
    # Dictionary mapping Trait Name -> Set of Files using it
    trait_to_files = defaultdict(set)

    # 1. Collect what is actually DECLARED
    if os.path.exists(DEFINITIONS_FILE):
        with open(DEFINITIONS_FILE, 'r') as f:
            content = f.read()
            matches = trait_decl_regex.findall(content)
            for m in matches:
                declared_traits.add(f"TRAIT_{m}")
    else:
        print(f"Error: Definitions file not found at {DEFINITIONS_FILE}")
        return

    # 2. Collect every TRAIT_ mention and track the source file
    for filepath in USAGE_FILES:
        if os.path.exists(filepath):
            with open(filepath, 'r') as f:
                # Read line by line or whole file; whole file is faster for regex
                content = f.read()
                matches = trait_usage_regex.findall(content)
                for m in matches:
                    trait_to_files[m].add(filepath)
        else:
            print(f"Warning: Usage file not found at {filepath}")

    # 3. Analyze
    all_used_traits = set(trait_to_files.keys())
    missing_definitions = all_used_traits - declared_traits
    unused_definitions = declared_traits - all_used_traits

    # Clean up results
    for trait in ["TRAIT_NIL", "TRAIT_DECL", "TRAIT_MODIFIER_DECL", "TRAIT_IGNORE_MODIFIER"]:
        missing_definitions.discard(trait)
        unused_definitions.discard(trait)

    print("="*40)
    print("         TRAIT SYSTEM AUDIT")
    print("="*40)
    
    if missing_definitions:
        print(f"\n[!] UNDECLARED TRAITS ({len(missing_definitions)})")
        print("These exist in code but NOT in Trait.enum:")
        print("-" * 40)
        for t in sorted(missing_definitions):
            sources = ", ".join(sorted(trait_to_files[t]))
            print(f" {t:<30} | Found in: {sources}")
    else:
        print("\n[+] All used traits are properly declared.")

    if unused_definitions:
        print(f"\n[*] GHOST TRAITS ({len(unused_definitions)})")
        print("These are declared but never used (safe to delete):")
        print("-" * 40)
        for t in sorted(unused_definitions):
            print(f" - {t}")
    else:
        print("\n[+] No unused traits found.")

if __name__ == "__main__":
    audit()