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
		player_CP->m_startTime = player->m_dashStartTime;
	}

	CheckpointObject* markCheckpoint() {

		PlayerObject* player1 = GJBaseGameLayer::get()->m_player1;
		PlayerObject* player2 = GJBaseGameLayer::get()->m_player2;

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
		startDashing(m_dashRing);
		m_flashTime = 0;
	}

	void update(float p0) {
		PlayerObject::update(p0);
		
		justRespawnedCP--;
		if (m_holdingButtons[1] && justRespawnedCP == 0) {
			dashFromCP();
		}
	}

	void loadFromCheckpoint(PlayerCheckpoint * checkpoint) {

		m_isDashing = checkpoint->m_dashing;
		m_dashX = checkpoint->m_dashX;
		m_dashY = checkpoint->m_dashY;
		m_dashRing = checkpoint->m_dashRingObject;
		m_dashAngle = checkpoint->m_dashAngle;
		m_dashStartTime = checkpoint->m_startTime;

		justRespawnedCP = 2;
		PlayerObject::loadFromCheckpoint(checkpoint);

		if (m_isDashing) {
			m_holdingButtons[1] = 2;
		}
	}

	bool releaseButton(PlayerButton button) {
		log::debug("releaseButton called.");
		if (justRespawnedCP >= 0) {
			bool isDashing_cache = m_isDashing;
			log::debug("isDashing_cache: {}", isDashing_cache);
			m_isDashing = false;

			bool jumpBuffered_cache = m_jumpBuffered;
			bool stateRingJump_cache = m_stateRingJump;
			bool stateRingJump2_cache = m_stateRingJump2;

			if (!isDashing_cache || button != PlayerButton::Jump) {
				log::debug("calling OG function...", isDashing_cache);
				if (!PlayerObject::releaseButton(button)) {
					m_isDashing = isDashing_cache;
					return false;
				}
			}
			else {
				log::debug("NO NEED to call OG function.", isDashing_cache);
				m_holdingButtons[1] = true;
			}

			m_isDashing = isDashing_cache;
			bool m_jumpBuffered = jumpBuffered_cache;
			bool m_stateRingJump = stateRingJump_cache;
			bool m_stateRingJump2 = stateRingJump2_cache;
		}
		else {
			if (!PlayerObject::releaseButton(button)) {
				return false;
			}
		}
		return true;
	}

	void stopDashing() {
		log::debug("stopDashing called.");
		log::debug("justRespawnedCP: {}", justRespawnedCP);

		if (justRespawnedCP < 0) {
			return PlayerObject::stopDashing();
		}
		justRespawnedCP -= 1;
	}
};

/*
class $modify(PCheckpointHook, PlayerCheckpoint) {
	struct Fields {
		DashRingObject* m_dashRing;
		bool m_isDashing;
		double m_dashX;
		double m_dashY;
		double m_dashAngle;
		double m_dashStartTime;
	};
};

class $modify(PLayerHook, PlayLayer) {

	struct Fields {
		int m_cpRespawnTimer = -1;
		bool m_p1WasDashing = false;
		bool m_p2WasDashing = false;
	};

	static void updatePlayerCP(PCheckpointHook * player_CP, PlayerObject * player) {
		player_CP->m_fields->m_isDashing = player->m_isDashing;
		player_CP->m_fields->m_dashX = player->m_dashX;
		player_CP->m_fields->m_dashY = player->m_dashY;
		player_CP->m_fields->m_dashRing = player->m_dashRing;
		player_CP->m_fields->m_dashStartTime = player->m_dashStartTime;
	}

	void makePlayerDash(PlayerObject* player) {
		player->startDashing(player->m_dashRing);
		player->m_flashTime = 0;
	}

	virtual void postUpdate(float p0) {

		PlayLayer::postUpdate(p0);

		if (m_fields->m_cpRespawnTimer >= 0) {
			if (m_fields->m_p1WasDashing) {
				if (m_player1->m_holdingButtons[1]) {
					makePlayerDash(m_player1);
				}
			}

			if (m_fields->m_p2WasDashing && m_player2) {
				if (m_player2->m_holdingButtons[1]) {
					makePlayerDash(m_player2);
				}
			}
		}
		m_fields->m_cpRespawnTimer -= 1;
	}

	CheckpointObject* markCheckpoint() {

		PlayerObject* player1 = GJBaseGameLayer::get()->m_player1;
		PlayerObject* player2 = GJBaseGameLayer::get()->m_player2;

		bool p1_dashing = player1->m_isDashing;
		player1->m_isDashing = false; // Workaround so the game actually places the checkpoint,
		bool p2_dashing = false;      // Since RobTop made it so you CAN'T PLACE CHECKPOINTS while dashing instead of fixing it. :|
		if (player2) {				  
			p2_dashing = player2->m_isDashing;
			player2->m_isDashing = false; 
		}

		CheckpointObject* checkpoint = PlayLayer::markCheckpoint();

		PCheckpointHook* player1_CP = static_cast<PCheckpointHook*>(checkpoint->m_player1Checkpoint);
		player1->m_isDashing = p1_dashing;
		updatePlayerCP(player1_CP, player1);

		if (player2 && checkpoint->m_player2Checkpoint) {
			player2->m_isDashing = p2_dashing;

			PCheckpointHook* player2_CP = static_cast<PCheckpointHook*>(checkpoint->m_player2Checkpoint);
			updatePlayerCP(player2_CP, player2);
		}


		return checkpoint;
	}
};

class $modify(PlayerObject) {

	void loadFromCheckpoint(PlayerCheckpoint* checkpoint) {

		PLayerHook* playLayerHook = static_cast<PLayerHook*>(GameManager::get()->m_playLayer);
		playLayerHook->m_fields->m_cpRespawnTimer = 1;

		PCheckpointHook* player_CP = static_cast<PCheckpointHook*>(checkpoint);
		bool wasDashing = player_CP->m_fields->m_isDashing;
		if (m_isSecondPlayer) {
			playLayerHook->m_fields->m_p2WasDashing = wasDashing;
		}
		else {
			playLayerHook->m_fields->m_p1WasDashing = wasDashing;
		}

		PlayerObject::loadFromCheckpoint(checkpoint);

		PCheckpointHook* cpHook = static_cast<PCheckpointHook*>(checkpoint);

		m_isDashing = cpHook->m_fields->m_isDashing;
		m_dashX = cpHook->m_fields->m_dashX;
		m_dashY = cpHook->m_fields->m_dashY;
		m_dashRing = cpHook->m_fields->m_dashRing;
		m_dashStartTime = cpHook->m_fields->m_dashStartTime;
	}
};
*/