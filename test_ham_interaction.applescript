-- AppleScript to test HAM MIDI routing functionality
-- This script attempts to interact with HAM's UI elements

tell application "System Events"
	-- Wait a moment for apps to be ready
	delay 2
	
	-- Try to find the HAM application window
	set hamFound to false
	repeat with appProcess in application processes
		if name of appProcess contains "Clone" or name of appProcess contains "HAM" then
			set hamFound to true
			set hamProcess to appProcess
			exit repeat
		end if
	end repeat
	
	if hamFound then
		log "Found HAM process: " & (name of hamProcess)
		
		-- Try to find UI elements
		tell hamProcess
			-- Look for windows
			set windowCount to count of windows
			log "HAM has " & windowCount & " windows"
			
			if windowCount > 0 then
				tell window 1
					-- Try to find any UI elements
					set elementCount to count of UI elements
					log "Main window has " & elementCount & " UI elements"
					
					-- In a real implementation, we would:
					-- 1. Find the TrackSidebar
					-- 2. Locate the MIDI routing dropdown
					-- 3. Change its value
					-- 4. Trigger playback
				end tell
			end if
		end tell
		
	else
		log "HAM application not found among running processes"
		
		-- List all running applications for debugging
		repeat with appProcess in application processes
			log "Found process: " & (name of appProcess)
		end repeat
	end if
end tell

return "AppleScript test completed"