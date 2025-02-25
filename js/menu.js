class Menu {
    constructor(board, panel) {
        this.panel = panel;
        this.board = board;
        this.gameMode;
        this.menuDiv = document.getElementById("menuPanel");
        document.getElementById("submitButton").onclick = () => {
            this.submit();
        };
        for (let id = 1; id <= 4; ++id) {
            document.getElementById(`sel${id}`)
                    .addEventListener("change", function () {
                        if (this.value == -1) {
                            document.getElementById("radWhite").style.cursor = "not-allowed";
                            document.getElementById("radBlack").style.cursor = "not-allowed";
                            document.getElementById("radRand").style.cursor = "not-allowed";
                            document.getElementById("selectWarning").style.opacity = "1";
                            document.getElementById( "selectWarning").style.visibility = "visible";
                        } else {
                            document.getElementById("radWhite").style.cursor = "pointer";
                            document.getElementById("radBlack").style.cursor = "pointer";
                            document.getElementById("radRand").style.cursor = "pointer";
                            document.getElementById("selectWarning").style.opacity = "0";
                            document.getElementById("selectWarning").style.visibility = "hidden";
                        }
                    });
        }
    }

    submit() {
        if (document.getElementById("sel1").checked == true) {
            this.gameMode = document.getElementById("sel1").value;
        } else if (document.getElementById("sel2").checked == true) {
            this.gameMode = document.getElementById("sel2").value;
        } else if (document.getElementById("sel3").checked == true) {
            this.gameMode = document.getElementById("sel3").value;
        } else if (document.getElementById("sel4").checked == true) {
            this.gameMode = document.getElementById("sel4").value;
        }

        this.color = [-1, 1][Math.floor(Math.random() * 2)];
        if (document.getElementById("selWhite").checked) {
            this.color = 1;
        } else if (document.getElementById("selBlack").checked) {
            this.color = -1;
        }

        // flip the board towards black if user selected to be black
        // except for the case where player selected player versus player
        if (this.color == -1 && this.gameMode != -1)
            this.board.flipTowards(-1);
        this.board.animateBoard();

        board.cpuDifficulty = this.gameMode;
        board.opponent = new Module.Solver(Number(board.cpuDifficulty));

        this.menuDiv.style.display = "none";
        document.getElementById("board").style.display = "block";
        this.panel.panelDiv.style.display = "block";
        this.panel.initialize(this.gameMode, this.color);
        document.getElementById("statusContainer").style.display = "block";
        document.getElementById("movesPanel").style.display = "block";
        document.getElementById("playAgain").style.display = "block";

        if (this.gameMode != -1) {
            if (this.color == 1) {
                this.board.changeClickability(true, false);
                this.board.aiColor = -1;
            } else {
                this.board.aiColor = 1;
                this.board.changeClickability(false, false);
                setTimeout(
                    () => this.board.getNextComputerMove(),
                    this.board.compDelay + 3000
                );
            }
        } else {
            this.board.changeClickability(true, false);
        }

        var gameStart = new Audio("sfx/game-start.wav");
        gameStart.play();
    }
}
