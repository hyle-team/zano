import {Component, NgZone, OnInit} from '@angular/core';
import {BackendService} from '../_helpers/services/backend.service';
import {VariablesService} from '../_helpers/services/variables.service';
import {Router} from '@angular/router';

@Component({
  selector: 'app-main',
  templateUrl: './main.component.html',
  styleUrls: ['./main.component.scss']
})
export class MainComponent implements OnInit {

  constructor(
    private router: Router,
    private backend: BackendService,
    private variablesService: VariablesService,
    private ngZone: NgZone
  ) {}

  ngOnInit() {}

  openWallet() {
    this.backend.openFileDialog('Open the wallet file.', '*', this.variablesService.settings.default_path, (file_status, file_data) => {
      if (file_status) {
        this.variablesService.settings.default_path = file_data.path.substr(0, file_data.path.lastIndexOf('/'));
        this.ngZone.run(() => {
          this.router.navigate(['/open'], {queryParams: {path: file_data.path}});
        });
      } else {
        console.log(file_data['error_code']);
      }
    });
  }

}
