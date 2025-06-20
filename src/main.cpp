#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/modify/PlayerCheckpoint.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

// NOTE TO SELF: m_platformerCheckpoint exists in PlayerCheckpoint. Could this be what I need for the setting I want to add?


class $modify(PlayLayer) {

	static void updatePlayerCP(PlayerCheckpoint * player_CP, PlayerObject * player) {
		player_CP->m_dashing = player->m_isDashing;
		player_CP->m_dashX = player->m_dashX;
		player_CP->m_dashY = player->m_dashY;
		player_CP->m_dashRingObject = player->m_dashRing;
		player_CP->m_dashAngle = player->m_dashAngle;
		player_CP->m_dashStartTime = player->m_dashStartTime;
	}

	CheckpointObject* markCheckpoint() {

		PlayerObject* player1 = GJBaseGameLayer::get()->m_player1;
		PlayerObject* player2 = GJBaseGameLayer::get()->m_player2;

		if (player1 == nullptr || player1->m_isDead) {
			return nullptr;
		}

		bool p1_dashing = player1->m_isDashing;
		player1->m_isDashing = false; // Workaround so the game actually places the checkpoint,
		bool p2_dashing = false;      // Since RobTop made it so you CAN'T PLACE CHECKPOINTS while dashing instead of fixing it. :|
		if (player2) {
			p2_dashing = player2->m_isDashing;
			player2->m_isDashing = false;
		}

		CheckpointObject* checkpoint = PlayLayer::markCheckpoint();

		player1->m_isDashing = p1_dashing;
		updatePlayerCP(checkpoint->m_player1Checkpoint, player1);

		if (player2 && checkpoint->m_player2Checkpoint) {
			player2->m_isDashing = p2_dashing;
			updatePlayerCP(checkpoint->m_player2Checkpoint, player2);
		}

		return checkpoint;
	}
};

int justRespawnedCP = -1;

class $modify(PlayerObject) {

	void dashFromCP() {
		bool hasNoEffects_cache = m_dashRing->m_hasNoEffects;
		m_dashRing->m_hasNoEffects = true;
		startDashing(m_dashRing);
		m_flashTime = 0;
		m_dashRing->m_hasNoEffects = hasNoEffects_cache;
	}

	void update(float p0) {
		PlayerObject::update(p0);
		
		if (!m_isSecondPlayer) {
			justRespawnedCP--;
		}
		if (m_holdingButtons[1] && justRespawnedCP == 0) {
			dashFromCP();
		}
	}

	void loadFromCheckpoint(PlayerCheckpoint * checkpoint) {

		bool affectPlat = Mod::get()->getSettingValue<bool>("affect-platformer-checkpoints");
		if (checkpoint->m_platformerCheckpoint && !affectPlat) {
			PlayerObject::loadFromCheckpoint(checkpoint);
			return;
		}

		m_isDashing = checkpoint->m_dashing;
		if (m_isDashing) {
			m_dashX = checkpoint->m_dashX;
			m_dashY = checkpoint->m_dashY;
			m_dashRing = checkpoint->m_dashRingObject;
			m_dashAngle = checkpoint->m_dashAngle;
			m_dashStartTime = checkpoint->m_dashStartTime;
			justRespawnedCP = 2;
		}
		PlayerObject::loadFromCheckpoint(checkpoint);

		if (m_isDashing) {
			m_holdingButtons[1] = 2;
		}
	}

	bool releaseButton(PlayerButton button) {
		if (justRespawnedCP >= 0) {
			bool isDashing_cache = m_isDashing;
			m_isDashing = false;

			bool jumpBuffered_cache = m_jumpBuffered;
			bool stateRingJump_cache = m_stateRingJump;
			bool stateRingJump2_cache = m_stateRingJump2;

			if (!isDashing_cache || button != PlayerButton::Jump) {
				if (!PlayerObject::releaseButton(button)) {
					m_isDashing = isDashing_cache;
					return false;
				}
			}
			else {
				m_holdingButtons[1] = true;
			}

			m_isDashing = isDashing_cache;
			bool m_jumpBuffered = jumpBuffered_cache;
			bool m_stateRingJump = stateRingJump_cache;
			bool m_stateRingJump2 = stateRingJump2_cache;

			if (m_isDashing) {
				PlayerObject::updateDashArt();
				m_dashParticles->setVisible(false);
				for (int i = 0; i < m_dashSpritesContainer->getChildrenCount(); i++) {
					CCSprite* sprite = (CCSprite*)m_dashSpritesContainer->getChildren()->objectAtIndex(i);
					sprite->setVisible(false);
				}
			}
		}
		else {
			if (!PlayerObject::releaseButton(button)) {
				return false;
			}
		}
		return true;
	}
};