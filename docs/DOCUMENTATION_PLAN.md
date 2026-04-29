# Documentation Modernization Plan

Document status: Proposed and partially implemented
Scope: Entire repository documentation surface
Owner: Mouhsine Kassimi Farhaoui <mouhsine98@gmail.com>
Last updated: 2026-04-28

## 1. Objectives

- Make onboarding fast for developers and operators.
- Capture architecture and runtime behavior with diagrams that clarify real decisions.
- Keep docs aligned with ROS 2 and GStreamer conventions.
- Define a maintainable docs lifecycle (ownership, versioning, and review gates).

## 2. Current-State Audit

Strengths:
- Solid parameter reference in README.
- Operational docs already exist for launch, troubleshooting, dependencies, and control plane.
- Good platform and release support material.

Gaps:
- No dedicated architecture document describing boundaries, components, and runtime flows.
- No docs index/hub to help users pick the right document quickly.
- Limited visual guidance for data flow, recovery behavior, and profile resolution.
- Author/ownership metadata is not consistently visible in docs.

## 3. Target Documentation Architecture

Mandatory docs:
- README: quickstart, capability summary, parameter reference, docs entry point.
- docs/INDEX.md: documentation hub and reading paths.
- docs/ARCHITECTURE.md: architecture decisions, components, and runtime behavior.
- docs/CONTROL_PLANE.md: contract-level API docs for topics and services.
- docs/LAUNCH.md: launch arguments and parameter mapping.
- docs/TROUBLESHOOTING.md: symptom-first operating playbook.
- docs/DEPENDENCIES.md: runtime and optional package requirements.
- docs/VERSIONING.md and CHANGELOG.md: release governance.

## 4. Diagram Strategy (Value-First)

Diagram set to maintain:
- System context diagram: camera drivers, bridge node, network receivers, and operators.
- Internal component diagram: config loader, pipeline builder, stream engine, metrics publisher.
- Runtime sequence diagram: frame ingestion, throttling, push path, backpressure/reconnect loop.
- Configuration resolution flow: profile defaults, YAML, CLI override precedence.

Rules:
- Keep diagrams source-of-truth in Mermaid for diff-friendly reviews.
- Every diagram must answer one operator or developer question.
- Update diagrams in the same PR as behavioral changes.

## 5. Conventions

Writing and structure:
- English only.
- Start each document with purpose and audience.
- Prefer task-oriented sections over long prose.
- Use consistent ROS naming and parameter notation.

Quality gates:
- Docs change required when adding parameters, launch args, messages, or runtime modes.
- Validate command snippets against Humble-compatible environments.
- Keep examples copy-paste ready.

## 6. Execution Plan

Phase 1 (completed in this update):
- Add docs/INDEX.md as a documentation hub.
- Add docs/ARCHITECTURE.md with professional diagrams and design rationale.
- Add author metadata and ownership visibility.
- Link new docs from README.

Phase 2:
- Add an operational runbook section for incident triage and escalation.
- Add architecture decision records (ADRs) for major pipeline/runtime choices.

Phase 3:
- Add release checklist automation for docs drift (parameter/launch/contract changes).

## 7. Maintenance Ownership

Primary maintainer:
- Mouhsine Kassimi Farhaoui <mouhsine98@gmail.com>

Recommended review policy:
- At least one technical reviewer for architecture-impacting changes.
- Require docs updates in PR template when public behavior changes.
