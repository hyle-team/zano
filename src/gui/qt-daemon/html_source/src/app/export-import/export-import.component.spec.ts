import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { ExportImportComponent } from './export-import.component';

describe('ExportImportComponent', () => {
  let component: ExportImportComponent;
  let fixture: ComponentFixture<ExportImportComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ ExportImportComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(ExportImportComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
