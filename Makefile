
.PHONY: clean
clean:
	@ rm -rf build/
	@ rm -rf __pycache__
	@ $(MAKE) -C src/ clean
