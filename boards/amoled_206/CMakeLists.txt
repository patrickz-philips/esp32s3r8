# Board component: carries the managed-component dependencies for this board only.
# Only the selected board directory is added via EXTRA_COMPONENT_DIRS, so the
# component manager installs just this board's dependencies.
idf_component_register(INCLUDE_DIRS ".")
